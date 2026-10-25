#include "kthread.h"

#include <errno.h>
#include <string.h>
#include <time.h>

/* TCB của thread đang chạy, giống _current của Zephyr */
static _Thread_local struct kthread *curr_thread;

int kthread_init_self(struct kthread *self, const char *name, int prio)
{
	if (!self || !name) {
		return -EINVAL;
	}

	memset(self, 0, sizeof(*self));
	self->name = name;
	self->prio = prio;
	pthread_mutex_init(&self->park_mtx, NULL);
	pthread_cond_init(&self->park_cv, NULL);
	curr_thread = self;
	return 0;
}

struct kthread *kthread_current(void)
{
	return curr_thread;
}

void wait_q_init(struct wait_q *q)
{
	q->head = NULL;
}

bool wait_q_is_empty(const struct wait_q *q)
{
	return q->head == NULL;
}

/* Chèn sau mọi thread có ưu tiên cao hơn HOẶC BẰNG: cùng ưu tiên thì ai đến
 * trước được đánh thức trước, không ai bị "chen hàng" */
static void wait_q_insert(struct wait_q *q, struct kthread *t)
{
	struct kthread **pp = &q->head;

	while (*pp && (*pp)->prio <= t->prio) {
		pp = &(*pp)->next;
	}
	t->next = *pp;
	*pp = t;
}

static void wait_q_remove(struct wait_q *q, struct kthread *t)
{
	for (struct kthread **pp = &q->head; *pp; pp = &(*pp)->next) {
		if (*pp == t) {
			*pp = t->next;
			t->next = NULL;
			return;
		}
	}
}

struct kthread *wait_q_unpend_first(struct wait_q *q)
{
	struct kthread *t = q->head;

	if (t) {
		q->head = t->next;
		t->next = NULL;
	}
	return t;
}

void kthread_ready(struct kthread *t, int retval)
{
	pthread_mutex_lock(&t->park_mtx);
	t->swap_retval = retval;
	t->woken = true;
	pthread_cond_signal(&t->park_cv);
	pthread_mutex_unlock(&t->park_mtx);
}

static struct timespec deadline_after_ms(int32_t ms)
{
	struct timespec ts;

	clock_gettime(CLOCK_REALTIME, &ts); /* pthread_cond_timedwait dùng giờ REALTIME */
	ts.tv_sec += ms / 1000;
	ts.tv_nsec += (long)(ms % 1000) * 1000000L;
	if (ts.tv_nsec >= 1000000000L) {
		ts.tv_sec++;
		ts.tv_nsec -= 1000000000L;
	}
	return ts;
}

int wait_q_pend_curr(struct wait_q *q, struct spinlock *lock, struct spinlock_key key,
		     int32_t timeout_ms)
{
	struct kthread *self = curr_thread;
	struct timespec deadline = deadline_after_ms(timeout_ms > 0 ? timeout_ms : 0);
	bool timed_out = false;
	int ret;

	if (!self) {
		/* Thread chưa đăng ký với kernel thì không có TCB để xếp hàng */
		spinlock_unlock(lock, key);
		return -EPERM;
	}

	self->woken = false;
	wait_q_insert(q, self);

	/*
	 * Khóa "cửa phòng ngủ" của mình TRƯỚC khi thả spinlock. Nếu một ISR give
	 * ngay sau khi spinlock được thả, kthread_ready() sẽ phải chờ cửa này, tức
	 * là chờ tới lúc ta đã thật sự nằm trong pthread_cond_wait. Nhờ vậy không
	 * có khe hở nào để tín hiệu đánh thức bị mất (lost wakeup).
	 */
	pthread_mutex_lock(&self->park_mtx);
	spinlock_unlock(lock, key);
	while (!self->woken && !timed_out) {
		if (timeout_ms == K_FOREVER) {
			pthread_cond_wait(&self->park_cv, &self->park_mtx);
		} else {
			timed_out = pthread_cond_timedwait(&self->park_cv, &self->park_mtx,
							   &deadline) == ETIMEDOUT;
		}
	}
	bool woken = self->woken;
	pthread_mutex_unlock(&self->park_mtx);

	if (woken) {
		return self->swap_retval;
	}

	/* Hết giờ: tự rút tên khỏi hàng đợi. Phải kiểm tra lại dưới spinlock vì
	 * có thể đúng lúc này có người vừa give và chọn trúng ta. */
	key = spinlock_lock(lock);
	if (self->woken) {
		ret = self->swap_retval;
	} else {
		wait_q_remove(q, self);
		ret = -EAGAIN;
	}
	spinlock_unlock(lock, key);
	return ret;
}
