#include "buf_pool.h"
#include "buf_queue.h"
#include "kthread.h"
#include "sem.h"
#include "sim_cpu.h"
#include "spinlock.h"
#include "uart_tx.h"

#include <errno.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

static double now_ms(void)
{
	struct timespec t;

	clock_gettime(CLOCK_MONOTONIC, &t);
	return t.tv_sec * 1000.0 + t.tv_nsec / 1e6;
}

static void sleep_ms(unsigned int ms)
{
	struct timespec t = { (time_t)(ms / 1000), (long)(ms % 1000) * 1000000L };

	nanosleep(&t, NULL);
}

/* ---------------------------------------------------------------------------
 * Demo 1: ví dụ của sách. Gửi UART rồi ngủ tới khi ngắt "TX complete" give.
 * ------------------------------------------------------------------------- */

static void demo_uart_tx(void)
{
	static const uint8_t cmd[] = "AT+CSQ\r\n";
	struct uart_tx uart;

	uart_tx_init(&uart, "UART1", 9600);

	double t0 = now_ms();
	int ret = uart_tx_transmit(&uart, cmd, sizeof(cmd) - 1, 100);
	printf("  transmit %u bytes @9600: ret=%d after %.1f ms (wire time %.1f ms)\n",
	       (unsigned)(sizeof(cmd) - 1), ret, now_ms() - t0,
	       (sizeof(cmd) - 1) * uart.byte_time_us / 1000.0);

	uart_tx_inject_fault(&uart);
	ret = uart_tx_transmit(&uart, cmd, sizeof(cmd) - 1, 100);
	printf("  transmit with line fault: ret=%d (-EIO=%d), thread did not hang\n", ret, -EIO);
}

/* ---------------------------------------------------------------------------
 * Demo 2: count nghĩa là gì (ba tình huống trong mục "Additional details").
 * ------------------------------------------------------------------------- */

static struct sem demo_sem;

static void *waiter_main(void *arg)
{
	struct kthread self;

	sim_cpu_bind(1);
	kthread_init_self(&self, "waiter", 5);
	int ret = sem_take(&demo_sem, K_FOREVER);
	*(int *)arg = ret;
	return NULL;
}

/* Cho một thread vào ngủ trên demo_sem, chờ nó thật sự nằm trong hàng đợi */
static pthread_t start_waiter(int *ret)
{
	pthread_t t;

	*ret = 1;
	pthread_create(&t, NULL, waiter_main, ret);
	sleep_ms(50);
	return t;
}

static void demo_count_semantics(void)
{
	int ret;
	pthread_t t;

	printf("  case A: init(0, limit 1), a thread waits, then give\n");
	sem_init(&demo_sem, 0, 1);
	t = start_waiter(&ret);
	sem_give(&demo_sem);
	pthread_join(t, NULL);
	printf("    waiter woke with ret=%d, count=%lu  (count stays 0)\n", ret,
	       (unsigned long)sem_count_get(&demo_sem));

	printf("  case B: init(0, limit 1), give, give, take\n");
	sem_init(&demo_sem, 0, 1);
	sem_give(&demo_sem);
	printf("    give      -> count=%lu\n", (unsigned long)sem_count_get(&demo_sem));
	sem_give(&demo_sem);
	printf("    give      -> count=%lu  (limit reached, give is dropped)\n",
	       (unsigned long)sem_count_get(&demo_sem));
	ret = sem_take(&demo_sem, K_NO_WAIT);
	printf("    take      -> ret=%d count=%lu\n", ret, (unsigned long)sem_count_get(&demo_sem));
	ret = sem_take(&demo_sem, K_NO_WAIT);
	printf("    take      -> ret=%d (-EBUSY=%d)\n", ret, -EBUSY);

	printf("  case C: init(2, limit 2)\n");
	sem_init(&demo_sem, 2, 2);
	sem_give(&demo_sem);
	printf("    give      -> count=%lu  (nothing changes)\n",
	       (unsigned long)sem_count_get(&demo_sem));
	sem_take(&demo_sem, K_NO_WAIT);
	sem_take(&demo_sem, K_NO_WAIT);
	printf("    take x2   -> count=%lu\n", (unsigned long)sem_count_get(&demo_sem));
	t = start_waiter(&ret);
	printf("    3rd take  -> thread is waiting, count=%lu\n",
	       (unsigned long)sem_count_get(&demo_sem));
	sem_give(&demo_sem);
	pthread_join(t, NULL);
	printf("    give      -> waiter woke (ret=%d), count=%lu\n", ret,
	       (unsigned long)sem_count_get(&demo_sem));
	sem_give(&demo_sem);
	printf("    give      -> nobody waits, count=%lu\n", (unsigned long)sem_count_get(&demo_sem));
}

/* ---------------------------------------------------------------------------
 * Demo 3: nhiều thread cùng chờ; ISR give 3 lần; ai được đánh thức trước?
 * ------------------------------------------------------------------------- */

struct prio_worker {
	const char *name;
	int prio;
	unsigned int cpu;
	pthread_t thread;
};

static struct sem event_sem;
static struct spinlock log_lock = SPINLOCK_INIT;
static const char *wake_log[3];
static size_t wake_count;

static void *prio_worker_main(void *arg)
{
	struct prio_worker *w = arg;
	struct kthread self;

	sim_cpu_bind(w->cpu);
	kthread_init_self(&self, w->name, w->prio);
	if (sem_take(&event_sem, K_FOREVER) == 0) {
		struct spinlock_key key = spinlock_lock(&log_lock);
		wake_log[wake_count++] = w->name;
		spinlock_unlock(&log_lock, key);
	}
	return NULL;
}

static void event_isr(void *arg)
{
	(void)arg;
	sem_give(&event_sem);
}

static void demo_priority_wakeup(void)
{
	struct prio_worker workers[] = {
		{ "logger(prio 5)", 5, 1, 0 },
		{ "motor(prio 1)", 1, 2, 0 },
		{ "ui(prio 3)", 3, 3, 0 },
	};
	struct sim_irq_config irq = {
		.cpu_id = 6, .preempt = false, .isr = event_isr, .fire_count = 3, .gap_us = 20000,
	};

	sem_init(&event_sem, 0, 1);
	wake_count = 0;
	for (size_t i = 0; i < 3; i++) {
		pthread_create(&workers[i].thread, NULL, prio_worker_main, &workers[i]);
		sleep_ms(20); /* vào hàng đợi theo thứ tự: logger, motor, ui */
	}
	printf("  waiting order: logger(prio 5), motor(prio 1), ui(prio 3)\n");
	sim_irq_start(&irq);
	sim_irq_wait(NULL);
	for (size_t i = 0; i < 3; i++) {
		pthread_join(workers[i].thread, NULL);
	}
	printf("  wake order   : %s, %s, %s\n", wake_log[0], wake_log[1], wake_log[2]);
}

/* ---------------------------------------------------------------------------
 * Demo 4: ISR give 5 lần trong lúc thread đang bận. Binary vs counting.
 * ------------------------------------------------------------------------- */

static void burst_isr(void *arg)
{
	sem_give(arg);
}

static void demo_event_collapse(uint32_t limit)
{
	struct sem sig;
	struct sim_irq_config irq = {
		.cpu_id = 6, .preempt = false, .isr = burst_isr, .arg = &sig, .fire_count = 5,
		.gap_us = 100,
	};
	int seen = 0;

	sem_init(&sig, 0, limit);
	sim_irq_start(&irq);
	sim_irq_wait(NULL); /* thread "bận" trong lúc 5 ngắt đến */
	while (sem_take(&sig, K_NO_WAIT) == 0) {
		seen++;
	}
	printf("  limit %2lu: isr gave 5 times, thread saw %d event(s)\n", (unsigned long)limit,
	       seen);
}

/* ---------------------------------------------------------------------------
 * Demo 5: UART RX theo dòng. ISR mượn buffer (K_NO_WAIT), đổ dữ liệu, đẩy vào
 * hàng đợi; 2 worker lấy ra xử lý rồi trả buffer.
 * ------------------------------------------------------------------------- */

#define RX_LINES 2000
#define RX_WORKERS 2

static struct buf_pool pool;
static struct buf_queue queue;
static uint32_t rx_seq;
static atomic_uint rx_dropped;
static atomic_uint rx_processed;
static atomic_uint rx_corrupt;

static void uart_rx_line_isr(void *arg)
{
	struct rx_buf *buf;

	(void)arg;
	/* ISR KHÔNG được chờ: hết buffer thì bỏ dòng này và đếm lại */
	if (buf_pool_get(&pool, &buf, K_NO_WAIT) != 0) {
		atomic_fetch_add(&rx_dropped, 1);
		rx_seq++;
		return;
	}
	/* "DMA" đổ một dòng: 4 byte số thứ tự + byte kiểm tra */
	memcpy(buf->data, &rx_seq, sizeof(rx_seq));
	buf->data[4] = (uint8_t)(rx_seq * 7u);
	buf->len = 5;
	rx_seq++;
	buf_queue_put(&queue, buf);
}

static void *rx_worker_main(void *arg)
{
	struct kthread self;
	struct rx_buf *buf;

	sim_cpu_bind((unsigned int)(size_t)arg);
	kthread_init_self(&self, "rx_worker", 4);
	for (;;) {
		buf_queue_get(&queue, &buf, K_FOREVER);
		if (!buf) {
			break; /* tin "dừng" */
		}
		uint32_t seq;
		memcpy(&seq, buf->data, sizeof(seq));
		if (buf->data[4] != (uint8_t)(seq * 7u)) {
			atomic_fetch_add(&rx_corrupt, 1);
		}
		sim_busy_wait_us(400); /* "xử lý" một dòng: parse lệnh, ghi log... */
		buf_pool_put(&pool, &buf);
		atomic_fetch_add(&rx_processed, 1);
	}
	return NULL;
}

static void demo_rx_lines(uint32_t gap_us)
{
	pthread_t workers[RX_WORKERS];
	struct sim_irq_config irq = {
		.cpu_id = 6, .preempt = false, .isr = uart_rx_line_isr, .fire_count = RX_LINES,
		.gap_us = gap_us,
	};

	buf_pool_init(&pool);
	buf_queue_init(&queue);
	rx_seq = 0;
	atomic_store(&rx_dropped, 0);
	atomic_store(&rx_processed, 0);
	atomic_store(&rx_corrupt, 0);
	for (size_t i = 0; i < RX_WORKERS; i++) {
		pthread_create(&workers[i], NULL, rx_worker_main, (void *)(i + 1));
	}

	sim_irq_start(&irq);
	sim_irq_wait(NULL);
	while (atomic_load(&rx_processed) + atomic_load(&rx_dropped) < RX_LINES) {
		sleep_ms(1);
	}
	for (size_t i = 0; i < RX_WORKERS; i++) {
		buf_queue_put(&queue, NULL);
	}
	for (size_t i = 0; i < RX_WORKERS; i++) {
		pthread_join(workers[i], NULL);
	}
	printf("  one line every %3lu us: processed=%4u dropped=%4u corrupt=%u"
	       "  max buffers in use=%lu/%d\n",
	       (unsigned long)gap_us, atomic_load(&rx_processed), atomic_load(&rx_dropped),
	       atomic_load(&rx_corrupt), (unsigned long)buf_pool_max_in_use(&pool),
	       BUF_POOL_COUNT);
}

/* ---------------------------------------------------------------------------
 * Demo 6: timeout và các lỗi trả về.
 * ------------------------------------------------------------------------- */

static int isr_take_ret;

static void take_in_isr(void *arg)
{
	isr_take_ret = sem_take(arg, K_FOREVER);
}

static void demo_timeouts(void)
{
	struct sem never;
	struct sim_irq_config irq = {
		.cpu_id = 6, .preempt = false, .isr = take_in_isr, .arg = &never, .fire_count = 1,
	};

	sem_init(&never, 0, 1);
	int ret = sem_take(&never, K_NO_WAIT);
	printf("  take(K_NO_WAIT)         -> %d (-EBUSY=%d)\n", ret, -EBUSY);

	double t0 = now_ms();
	ret = sem_take(&never, 30);
	printf("  take(30 ms)             -> %d (-EAGAIN=%d) after %.1f ms\n", ret, -EAGAIN,
	       now_ms() - t0);

	sim_irq_start(&irq);
	sim_irq_wait(NULL);
	printf("  take(K_FOREVER) in ISR  -> %d (-EINVAL=%d)\n", isr_take_ret, -EINVAL);
}

int main(void)
{
	struct kthread self;

	sim_cpu_bind(0);
	kthread_init_self(&self, "main", 7);

	printf("1) blocking transmit, woken by the TX-complete interrupt\n");
	demo_uart_tx();

	printf("2) what the count means\n");
	demo_count_semantics();

	printf("3) three threads wait, ISR gives three times\n");
	demo_priority_wakeup();

	printf("4) five gives while the thread is busy\n");
	demo_event_collapse(1);
	demo_event_collapse(10);

	printf("5) UART RX lines: ISR -> buffer pool -> queue -> %d workers\n", RX_WORKERS);
	demo_rx_lines(300);
	demo_rx_lines(100);

	printf("6) timeouts and errors\n");
	demo_timeouts();
	return 0;
}
