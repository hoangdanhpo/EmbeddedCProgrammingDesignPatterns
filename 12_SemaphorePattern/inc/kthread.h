#ifndef KTHREAD_H
#define KTHREAD_H

#include "spinlock.h"

#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>

/*
 * "Kernel" thu nhỏ chạy trên PC: thứ mà semaphore cần từ bộ lập lịch.
 *
 * Trong RTOS thật (Zephyr), mỗi thread có một TCB (struct k_thread) và kernel
 * có thể: đưa thread hiện tại vào hàng đợi rồi chuyển CPU sang thread khác
 * (z_pend_curr), lấy thread ưu tiên cao nhất ra khỏi hàng đợi
 * (z_unpend_first_thread), đánh dấu nó sẵn sàng chạy (z_ready_thread).
 *
 * Trên PC, "chuyển CPU sang thread khác" là việc của Windows. Ta mô phỏng việc
 * ngủ/thức của từng thread bằng một cặp pthread mutex + condvar riêng của thread
 * đó (park_mtx/park_cv). Đây chỉ là phần thay thế bộ lập lịch, không phải là
 * mutex/condvar của bài 13-14.
 */

#define K_NO_WAIT 0     /* không chờ: trả lỗi ngay nếu chưa lấy được */
#define K_FOREVER (-1)  /* chờ tới khi lấy được */

/* TCB giả lập: mỗi thread tự cấp phát (trên stack của chính nó) */
struct kthread {
	const char *name;
	int prio;              /* số nhỏ = ưu tiên cao, giống Zephyr */
	struct kthread *next;  /* móc vào wait queue (danh sách liên kết nội tại) */
	bool woken;            /* đã được lấy ra khỏi wait queue và đánh thức */
	int swap_retval;       /* giá trị pend_curr trả về khi được đánh thức */
	pthread_mutex_t park_mtx;
	pthread_cond_t park_cv;
};

/* Hàng đợi các thread đang chờ, sắp theo ưu tiên (cùng ưu tiên thì FIFO) */
struct wait_q {
	struct kthread *head;
};

/* Đăng ký thread đang chạy với "kernel" (gọi một lần ở đầu thread) */
int kthread_init_self(struct kthread *self, const char *name, int prio);
struct kthread *kthread_current(void);

void wait_q_init(struct wait_q *q);
bool wait_q_is_empty(const struct wait_q *q);

/*
 * Giống z_pend_curr(): đưa thread hiện tại vào q, THẢ spinlock và ngủ, tất cả
 * như một bước không thể chen ngang. Caller phải đang giữ lock (với key).
 * Trả về swap_retval khi được đánh thức, -EAGAIN khi hết thời gian chờ.
 * Khi hàm return, lock đã được thả.
 */
int wait_q_pend_curr(struct wait_q *q, struct spinlock *lock, struct spinlock_key key,
		     int32_t timeout_ms);

/* Giống z_unpend_first_thread(): lấy thread ưu tiên cao nhất ra, NULL nếu rỗng.
 * Caller phải giữ spinlock bảo vệ q. */
struct kthread *wait_q_unpend_first(struct wait_q *q);

/* Giống arch_thread_return_value_set() + z_ready_thread(): đánh thức t,
 * làm cho pend_curr của t trả về retval. Caller phải giữ spinlock bảo vệ q. */
void kthread_ready(struct kthread *t, int retval);

#endif /* KTHREAD_H */
