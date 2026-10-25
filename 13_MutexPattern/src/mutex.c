#include "mutex.h"

#include <errno.h>
#include <time.h>

int mutex_init(struct mutex *self)
{
	if (!self) {
		return -EINVAL;
	}

	if (pthread_mutex_init(&self->guard, NULL) != 0) {
		return -ENOMEM;
	}
	self->lock_count = 0;
	self->head = NULL;
	self->tail = NULL;
	self->contentions = 0;
	return 0;
}

int mutex_deinit(struct mutex *self)
{
	if (!self) {
		return -EINVAL;
	}

	pthread_mutex_lock(&self->guard);
	/* Hủy mutex khi còn người giữ hoặc đang chờ thì họ sẽ dùng bộ nhớ đã chết */
	bool busy = self->lock_count != 0 || self->head != NULL;
	pthread_mutex_unlock(&self->guard);
	if (busy) {
		return -EBUSY;
	}

	pthread_mutex_destroy(&self->guard);
	return 0;
}

/* Các hàm hàng đợi chỉ được gọi khi đang giữ guard */
static void queue_push(struct mutex *self, struct mutex_waiter *w)
{
	w->next = NULL;
	if (self->tail) {
		self->tail->next = w;
	} else {
		self->head = w;
	}
	self->tail = w;
}

static struct mutex_waiter *queue_pop(struct mutex *self)
{
	struct mutex_waiter *w = self->head;

	if (w) {
		self->head = w->next;
		if (!self->head) {
			self->tail = NULL;
		}
	}
	return w;
}

static void queue_remove(struct mutex *self, struct mutex_waiter *w)
{
	struct mutex_waiter *prev = NULL;

	for (struct mutex_waiter *it = self->head; it; prev = it, it = it->next) {
		if (it != w) {
			continue;
		}
		if (prev) {
			prev->next = it->next;
		} else {
			self->head = it->next;
		}
		if (self->tail == it) {
			self->tail = prev;
		}
		return;
	}
}

/* pthread_cond_timedwait cần mốc thời gian tuyệt đối, không phải khoảng chờ */
static struct timespec deadline_after_ms(int32_t timeout_ms)
{
	struct timespec ts;

	timespec_get(&ts, TIME_UTC);
	ts.tv_sec += timeout_ms / 1000;
	ts.tv_nsec += (long)(timeout_ms % 1000) * 1000000L;
	if (ts.tv_nsec >= 1000000000L) {
		ts.tv_sec += 1;
		ts.tv_nsec -= 1000000000L;
	}
	return ts;
}

int mutex_lock(struct mutex *self, int32_t timeout_ms)
{
	if (!self || timeout_ms < MUTEX_FOREVER) {
		return -EINVAL;
	}

	pthread_t me = pthread_self();

	pthread_mutex_lock(&self->guard);

	/* Đường nhanh 1: mutex đang mở (khi còn waiter thì mutex không bao giờ ở trạng thái mở) */
	if (self->lock_count == 0) {
		self->lock_count = 1;
		self->owner = me;
		pthread_mutex_unlock(&self->guard);
		return 0;
	}

	/* Đường nhanh 2: chính mình đang giữ, chỉ tăng bộ đếm khóa lồng */
	if (pthread_equal(self->owner, me)) {
		self->lock_count++;
		pthread_mutex_unlock(&self->guard);
		return 0;
	}

	if (timeout_ms == MUTEX_NO_WAIT) {
		pthread_mutex_unlock(&self->guard);
		return -EBUSY;
	}

	/* Đường chậm: vào hàng đợi và ngủ cho tới khi được trao quyền hoặc hết giờ */
	struct mutex_waiter w = { .thread = me, .granted = false, .next = NULL };
	struct timespec deadline = deadline_after_ms(timeout_ms > 0 ? timeout_ms : 0);
	int ret = 0;

	pthread_cond_init(&w.wake, NULL);
	queue_push(self, &w);
	self->contentions++;

	/* while, không phải if: pthread được phép đánh thức "nhầm" (spurious wakeup) */
	while (!w.granted) {
		if (timeout_ms == MUTEX_FOREVER) {
			pthread_cond_wait(&w.wake, &self->guard);
		} else if (pthread_cond_timedwait(&w.wake, &self->guard, &deadline) == ETIMEDOUT &&
			   !w.granted) {
			queue_remove(self, &w);
			ret = -EAGAIN;
			break;
		}
	}
	/* Nếu granted: người unlock đã đặt sẵn owner = me và lock_count = 1 */

	pthread_mutex_unlock(&self->guard);
	pthread_cond_destroy(&w.wake);
	return ret;
}

int mutex_unlock(struct mutex *self)
{
	if (!self) {
		return -EINVAL;
	}

	pthread_mutex_lock(&self->guard);

	if (self->lock_count == 0) {
		pthread_mutex_unlock(&self->guard);
		return -EINVAL;
	}
	/* Đây là điểm khác biệt cốt lõi với semaphore: chỉ owner mới được mở */
	if (!pthread_equal(self->owner, pthread_self())) {
		pthread_mutex_unlock(&self->guard);
		return -EPERM;
	}

	if (self->lock_count > 1) {
		self->lock_count--;
		pthread_mutex_unlock(&self->guard);
		return 0;
	}

	struct mutex_waiter *next = queue_pop(self);
	if (next) {
		/*
		 * Trao quyền trực tiếp (handoff): mutex không mở ra lúc nào cả, nên thread
		 * vừa unlock không thể quay lại giành trước người đã xếp hàng.
		 */
		self->owner = next->thread;
		next->granted = true;
		pthread_cond_signal(&next->wake);
	} else {
		self->lock_count = 0;
	}

	pthread_mutex_unlock(&self->guard);
	return 0;
}

int mutex_lock_depth(struct mutex *self)
{
	if (!self) {
		return -EINVAL;
	}

	pthread_mutex_lock(&self->guard);
	int depth = (self->lock_count != 0 && pthread_equal(self->owner, pthread_self())) ?
			    (int)self->lock_count :
			    -EPERM;
	pthread_mutex_unlock(&self->guard);
	return depth;
}
