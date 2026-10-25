#include "arch_irq.h"
#include "rx_ring.h"
#include "sim_cpu.h"
#include "spinlock.h"

#include <errno.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>

/* Rào compiler: cấm gcc gộp load + store thành một lệnh "add [mem], n" của x86 */
#define COMPILER_BARRIER() __asm__ __volatile__("" ::: "memory")

/*
 * Cortex-M không cộng thẳng vào RAM được: "x += n" luôn là ldr, add, str.
 * Hàm này ép PC làm đúng ba bước như vậy để thấy được race giống trên MCU.
 */
static void add_like_cortex_m(int *p, int n)
{
	int v = *p;          /* ldr r3, [r2]      */
	COMPILER_BARRIER();
	v += n;              /* add r3, r3, #n    */
	*p = v;              /* str r3, [r2]      */
}

/* ---------------------------------------------------------------------------
 * Demo 1: ví dụ của sách. main cộng 8, ISR cộng 5 vào cùng một biến.
 * ------------------------------------------------------------------------- */

struct data {
	int foo;
};

static struct data data;
static struct spinlock data_lock = SPINLOCK_INIT;

static void data_isr(void *arg)
{
	(void)arg;
	add_like_cortex_m(&data.foo, 5);
}

enum protect { PROTECT_NONE, PROTECT_IRQ_LOCK, PROTECT_SPINLOCK };

static const char *protect_name[] = { "no lock", "irq_lock", "spinlock" };

static void demo_isr_race(enum protect how)
{
	struct sim_irq_config cfg = {
		.cpu_id = 0, .preempt = true, .isr = data_isr, .fire_count = 10000, .gap_us = 20,
	};
	uint32_t handled = 0;
	long long main_adds = 0;

	data.foo = 0;
	sim_irq_start(&cfg);
	while (!sim_irq_done()) {
		if (how == PROTECT_IRQ_LOCK) {
			unsigned int key = arch_irq_lock();
			add_like_cortex_m(&data.foo, 8);
			arch_irq_unlock(key);
		} else if (how == PROTECT_SPINLOCK) {
			struct spinlock_key key = spinlock_lock(&data_lock);
			add_like_cortex_m(&data.foo, 8);
			spinlock_unlock(&data_lock, key);
		} else {
			add_like_cortex_m(&data.foo, 8);
		}
		main_adds++;
	}
	sim_irq_wait(&handled);

	long long expected = 8 * main_adds + 5LL * handled;
	printf("  %-8s: isr=%5lu  expected=%10lld  actual=%10d  lost isr updates=%lld\n",
	       protect_name[how], (unsigned long)handled, expected, data.foo,
	       (expected - data.foo) / 5);
}

/* ---------------------------------------------------------------------------
 * Demo 2: UART RX ring buffer. ISR đẩy byte 0,1,2,... vào, main đọc ra và
 * kiểm tra thứ tự.
 * ------------------------------------------------------------------------- */

static struct rx_ring ring;
static bool ring_use_lock;
static uint8_t uart_dr;  /* "thanh ghi dữ liệu" UART giả lập: byte tiếp theo */
static uint32_t uart_sent;

static void uart_rx_isr(void *arg)
{
	(void)arg;
	int ret = ring_use_lock ? rx_ring_put(&ring, uart_dr) : rx_ring_put_nolock(&ring, uart_dr);

	/* Byte bị bỏ vì đầy thì phần cứng giả lập gửi lại ở ngắt sau, để phía main
	 * chỉ cần kiểm tra thứ tự liên tục */
	if (ret == 0) {
		uart_dr++;
		uart_sent++;
	}
}

static int ring_get(uint8_t *byte)
{
	return ring_use_lock ? rx_ring_get(&ring, byte) : rx_ring_get_nolock(&ring, byte);
}

static void demo_rx_ring(bool use_lock)
{
	struct sim_irq_config cfg = {
		.cpu_id = 0, .preempt = true, .isr = uart_rx_isr, .fire_count = 20000, .gap_us = 20,
	};
	uint32_t received = 0;
	uint32_t seq_errors = 0;
	uint8_t expect = 0;
	uint8_t byte;

	rx_ring_init(&ring);
	ring_use_lock = use_lock;
	uart_dr = 0;
	uart_sent = 0;
	sim_irq_start(&cfg);
	while (!sim_irq_done()) {
		/* main loop bận việc khác một lúc, rồi đọc hết những gì đã nhận */
		sim_busy_wait_us(100);
		while (ring_get(&byte) == 0) {
			received++;
			if (byte != expect) {
				seq_errors++;
			}
			expect = (uint8_t)(byte + 1);
		}
	}
	sim_irq_wait(NULL);

	printf("  %-8s: sent=%5lu  received=%5lu  sequence errors=%4lu  ring.count=%2lu"
	       "  (bytes really left=%lu)\n",
	       use_lock ? "spinlock" : "no lock", (unsigned long)uart_sent,
	       (unsigned long)received, (unsigned long)seq_errors, (unsigned long)ring.count,
	       (unsigned long)(uart_sent - received));
}

/* ---------------------------------------------------------------------------
 * Demo 3: hai CPU (hai thread song song thật) cùng cộng vào một biến.
 * Che ngắt chỉ có tác dụng trên CPU của mình, nên irq_lock không đủ.
 * ------------------------------------------------------------------------- */

#define SMP_ADDS 1000000

static enum protect smp_how;
static atomic_bool smp_go;

static void smp_work(void)
{
	while (!atomic_load(&smp_go)) {
	}
	for (int i = 0; i < SMP_ADDS; i++) {
		if (smp_how == PROTECT_SPINLOCK) {
			struct spinlock_key key = spinlock_lock(&data_lock);
			data.foo += 1;
			spinlock_unlock(&data_lock, key);
		} else {
			unsigned int key = arch_irq_lock();
			data.foo += 1; /* x86: một lệnh "add [mem], 1" nhưng không có tiền tố lock */
			arch_irq_unlock(key);
		}
	}
}

static void *cpu1_main(void *arg)
{
	(void)arg;
	sim_cpu_bind(1);
	smp_work();
	return NULL;
}

static void demo_smp(enum protect how)
{
	pthread_t cpu1;

	data.foo = 0;
	smp_how = how;
	atomic_store(&smp_go, false);
	pthread_create(&cpu1, NULL, cpu1_main, NULL);
	atomic_store(&smp_go, true);
	smp_work(); /* CPU 0 (main) làm cùng lúc với CPU 1 */
	pthread_join(cpu1, NULL);

	printf("  %-8s: expected=%d  actual=%d  lost=%d\n", protect_name[how], 2 * SMP_ADDS,
	       data.foo, 2 * SMP_ADDS - data.foo);
}

/* ---------------------------------------------------------------------------
 * Demo 4: key cho phép lồng hai spinlock KHÁC NHAU.
 * ------------------------------------------------------------------------- */

static void demo_nested(void)
{
	static struct spinlock a = SPINLOCK_INIT;
	static struct spinlock b = SPINLOCK_INIT;

	printf("  start          : irq locked=%d\n", arch_irq_is_locked());
	struct spinlock_key ka = spinlock_lock(&a);
	printf("  lock A (key=%u) : irq locked=%d\n", ka.key, arch_irq_is_locked());
	struct spinlock_key kb = spinlock_lock(&b);
	printf("  lock B (key=%u) : irq locked=%d\n", kb.key, arch_irq_is_locked());
	spinlock_unlock(&b, kb);
	printf("  unlock B       : irq locked=%d  <- still locked, A is held\n",
	       arch_irq_is_locked());
	spinlock_unlock(&a, ka);
	printf("  unlock A       : irq locked=%d\n", arch_irq_is_locked());
}

/* ---------------------------------------------------------------------------
 * Demo 5: vì sao spinlock phải che ngắt TRƯỚC khi quay. Lock "chỉ quay"
 * (không che ngắt) bị ISR trên cùng CPU chen vào lúc main đang giữ.
 * ------------------------------------------------------------------------- */

#define SPIN_LIMIT 50000000L /* quay quá ngần này coi như deadlock */

static atomic_int naive_lock;
static atomic_bool isr_deadlocked;
static atomic_int isr_ran;

static int naive_lock_bounded(void)
{
	for (long spins = 0; spins < SPIN_LIMIT; spins++) {
		int expected = 0;
		if (atomic_compare_exchange_strong(&naive_lock, &expected, 1)) {
			return 0;
		}
	}
	return -EDEADLK;
}

static void naive_isr(void *arg)
{
	bool use_spinlock = *(bool *)arg;

	if (use_spinlock) {
		struct spinlock_key key = spinlock_lock(&data_lock);
		data.foo++;
		spinlock_unlock(&data_lock, key);
	} else if (naive_lock_bounded() == 0) {
		data.foo++;
		atomic_store(&naive_lock, 0);
	} else {
		/* Trên MCU thật ISR sẽ quay mãi mãi: main bị nó chen ngang nên không
		 * bao giờ chạy tiếp để thả lock */
		atomic_store(&isr_deadlocked, true);
	}
	atomic_fetch_add(&isr_ran, 1);
}

static void demo_isr_deadlock(bool use_spinlock)
{
	struct sim_irq_config cfg = {
		.cpu_id = 0, .preempt = true, .isr = naive_isr, .arg = &use_spinlock,
		.fire_count = 1, .gap_us = 2000,
	};

	atomic_store(&isr_deadlocked, false);
	atomic_store(&isr_ran, 0);
	data.foo = 0;
	sim_irq_start(&cfg);

	/* main giữ lock khoảng 10 ms, ngắt đến sau 2 ms, tức là đúng lúc đang giữ */
	if (use_spinlock) {
		struct spinlock_key key = spinlock_lock(&data_lock);
		sim_busy_wait_us(10000);
		printf("  spinlock: main still in critical section, isr ran=%d (pending)\n",
		       atomic_load(&isr_ran));
		spinlock_unlock(&data_lock, key); /* ISR pending chạy ngay tại đây */
	} else {
		naive_lock_bounded();
		sim_busy_wait_us(10000);
		atomic_store(&naive_lock, 0);
	}
	sim_irq_wait(NULL);

	printf("  %-8s: isr ran=%d  isr deadlocked=%s  foo=%d\n",
	       use_spinlock ? "spinlock" : "spin only", atomic_load(&isr_ran),
	       atomic_load(&isr_deadlocked) ? "YES" : "no", data.foo);
}

int main(void)
{
	sim_cpu_bind(0); /* thread main đóng vai CPU 0 */

	printf("1) main += 8 vs ISR += 5 on one CPU (ldr/add/str like Cortex-M)\n");
	demo_isr_race(PROTECT_NONE);
	demo_isr_race(PROTECT_IRQ_LOCK);
	demo_isr_race(PROTECT_SPINLOCK);

	printf("2) UART RX ring buffer: ISR producer, main consumer\n");
	demo_rx_ring(false);
	demo_rx_ring(true);

	printf("3) two CPUs, %d increments each\n", SMP_ADDS);
	demo_smp(PROTECT_IRQ_LOCK);
	demo_smp(PROTECT_SPINLOCK);

	printf("4) nested spinlocks and the key\n");
	demo_nested();

	printf("5) ISR on the same CPU wants a lock that main holds\n");
	demo_isr_deadlock(false);
	demo_isr_deadlock(true);
	return 0;
}
