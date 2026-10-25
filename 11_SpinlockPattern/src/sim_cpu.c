#include "arch_irq.h"
#include "sim_cpu.h"

#include <errno.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#else
#error "sim_cpu.c only supports Windows (SuspendThread)"
#endif

/* Trạng thái của một CPU giả lập: thứ mà phần cứng thật giữ trong thanh ghi */
struct sim_cpu {
	atomic_uint primask;   /* 1 = đang che ngắt (giống PRIMASK của Cortex-M) */
	atomic_bool in_isr;    /* đang chạy ISR (giống IPSR != 0) */
	atomic_bool pending;   /* có ngắt đến lúc đang che, chờ được chạy */
	HANDLE thread;         /* thread của PC đóng vai CPU này */
	bool bound;
};

/* Nguồn ngắt giả lập: phần cứng chỉ có một, nên dùng một instance tĩnh */
struct sim_irq {
	struct sim_irq_config cfg;
	pthread_t hw_thread;
	atomic_bool running;
	atomic_bool done;
	atomic_uint handled;
};

static struct sim_cpu sim_cpus[SIM_CPU_MAX];
static struct sim_irq sim_irq;

/* Mỗi thread của PC biết mình đang đóng vai CPU nào */
static _Thread_local struct sim_cpu *curr_cpu;

static struct sim_cpu *cpu_self(void)
{
	if (!curr_cpu) {
		/* Lỗi lập trình của demo: arch_* chỉ có nghĩa khi đã biết CPU nào */
		fprintf(stderr, "sim_cpu: thread did not call sim_cpu_bind()\n");
		abort();
	}
	return curr_cpu;
}

int sim_cpu_bind(unsigned int cpu_id)
{
	if (cpu_id >= SIM_CPU_MAX) {
		return -EINVAL;
	}

	struct sim_cpu *cpu = &sim_cpus[cpu_id];

	if (cpu->bound) {
		CloseHandle(cpu->thread); /* CPU này được gán cho thread mới */
	}

	/* GetCurrentThread() chỉ là handle giả, phải nhân bản thành handle thật
	 * thì thread phần cứng mới SuspendThread() được */
	if (!DuplicateHandle(GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(),
			     &cpu->thread, 0, FALSE, DUPLICATE_SAME_ACCESS)) {
		return -EIO;
	}
	atomic_store(&cpu->primask, 0);
	atomic_store(&cpu->in_isr, false);
	atomic_store(&cpu->pending, false);
	cpu->bound = true;
	curr_cpu = cpu;
	return 0;
}

/* Chạy ISR trong ngữ cảnh của CPU cpu (thread gọi tạm "mượn" CPU đó) */
static void run_isr(struct sim_cpu *cpu)
{
	struct sim_cpu *saved = curr_cpu;

	curr_cpu = cpu;
	atomic_store(&cpu->in_isr, true);
	sim_irq.cfg.isr(sim_irq.cfg.arg);
	atomic_store(&cpu->in_isr, false);
	atomic_fetch_add(&sim_irq.handled, 1);
	curr_cpu = saved;
}

unsigned int arch_irq_lock(void)
{
	/* Đọc trạng thái cũ và che ngắt trong MỘT thao tác, như "mrs + cpsid i" */
	return atomic_exchange(&cpu_self()->primask, 1);
}

void arch_irq_unlock(unsigned int key)
{
	struct sim_cpu *cpu = cpu_self();

	if (key != 0) {
		/* Trước khi lock ngắt đã bị che: giữ nguyên, người lock ngoài cùng sẽ mở */
		return;
	}
	atomic_store(&cpu->primask, 0);

	/* Phần cứng thật: ngắt pending được phục vụ ngay khi mở ngắt. Nếu chính
	 * ta đang ở trong ISR thì chưa: một ngắt không chen được vào chính nó, nó
	 * sẽ chạy sau khi ISR hiện tại return (vòng lặp của lớp unlock ngoài). */
	if (atomic_load(&cpu->in_isr)) {
		return;
	}
	while (atomic_exchange(&cpu->pending, false)) {
		run_isr(cpu);
	}
}

bool arch_irq_is_locked(void)
{
	return atomic_load(&cpu_self()->primask) != 0;
}

bool arch_is_in_isr(void)
{
	return curr_cpu && atomic_load(&curr_cpu->in_isr);
}

unsigned int arch_curr_cpu_id(void)
{
	return (unsigned int)(cpu_self() - sim_cpus);
}

void sim_busy_wait_us(uint32_t us)
{
	struct timespec start;
	struct timespec now;

	clock_gettime(CLOCK_MONOTONIC, &start);
	do {
		clock_gettime(CLOCK_MONOTONIC, &now);
	} while ((now.tv_sec - start.tv_sec) * 1000000L + (now.tv_nsec - start.tv_nsec) / 1000L <
		 (long)us);
}

/* Một lần "chân IRQ được kích": chen ngang CPU đích như phần cứng thật */
static void fire_preempt(struct sim_cpu *cpu)
{
	/* Ngắt trước còn pending thì phần cứng chưa phát ngắt mới (một bit pending) */
	while (atomic_load(&cpu->pending)) {
	}

	SuspendThread(cpu->thread);
	/* SuspendThread là bất đồng bộ; GetThreadContext buộc chờ tới khi thread
	 * thật sự đứng yên. Từ đây CPU đích dừng ở một lệnh máy bất kỳ. */
	CONTEXT ctx;
	ctx.ContextFlags = CONTEXT_CONTROL;
	GetThreadContext(cpu->thread, &ctx);

	/* Đang che ngắt, hoặc đang chạy ISR này rồi (ngắt không lồng vào chính nó):
	 * ngắt chuyển sang pending */
	if (atomic_load(&cpu->primask) || atomic_load(&cpu->in_isr)) {
		atomic_store(&cpu->pending, true);
	} else {
		run_isr(cpu);
	}
	ResumeThread(cpu->thread);
}

static void *hw_thread_main(void *arg)
{
	struct sim_irq *irq = arg;
	struct sim_cpu *cpu = &sim_cpus[irq->cfg.cpu_id];

	if (!irq->cfg.preempt) {
		/* Chế độ song song: thread phần cứng đóng vai luôn CPU xử lý ngắt */
		sim_cpu_bind(irq->cfg.cpu_id);
	}

	for (uint32_t i = 0; i < irq->cfg.fire_count; i++) {
		if (irq->cfg.gap_us) {
			sim_busy_wait_us(irq->cfg.gap_us);
		}
		if (irq->cfg.preempt) {
			fire_preempt(cpu);
		} else {
			run_isr(cpu);
		}
	}

	/* Chờ ngắt pending cuối cùng (nếu có) được CPU đích phục vụ */
	while (atomic_load(&cpu->pending)) {
	}
	atomic_store(&irq->done, true);
	return NULL;
}

int sim_irq_start(const struct sim_irq_config *cfg)
{
	if (!cfg || !cfg->isr || cfg->cpu_id >= SIM_CPU_MAX) {
		return -EINVAL;
	}
	if (cfg->preempt && !sim_cpus[cfg->cpu_id].bound) {
		return -ENODEV;
	}
	if (atomic_exchange(&sim_irq.running, true)) {
		return -EBUSY;
	}

	sim_irq.cfg = *cfg;
	atomic_store(&sim_irq.done, false);
	atomic_store(&sim_irq.handled, 0);
	if (pthread_create(&sim_irq.hw_thread, NULL, hw_thread_main, &sim_irq) != 0) {
		atomic_store(&sim_irq.running, false);
		return -EAGAIN;
	}
	return 0;
}

bool sim_irq_done(void)
{
	return atomic_load(&sim_irq.done);
}

int sim_irq_wait(uint32_t *handled)
{
	if (!atomic_load(&sim_irq.running)) {
		return -EINVAL;
	}

	pthread_join(sim_irq.hw_thread, NULL);
	if (handled) {
		*handled = atomic_load(&sim_irq.handled);
	}
	atomic_store(&sim_irq.running, false);
	return 0;
}
