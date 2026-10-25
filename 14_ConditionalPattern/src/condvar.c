#include "condvar.h"

#include <errno.h>
#include <time.h>

int condvar_init(struct condvar *self)
{
	if (!self) {
		return -EINVAL;
	}

	if (pthread_mutex_init(&self->guard, NULL) != 0) {
		return -ENOMEM;
	}
	self->head = NULL;
	self->tail = NULL;
	return 0;
}

int condvar_deinit(struct condvar *self)
{
	if (!self) {
		return -EINVAL;
	}

	pthread_mutex_lock(&self->guard);
	bool busy = self->head != NULL;
	pthread_mutex_unlock(&self->guard);
	if (busy) {
		return -EBUSY; /* Còn thread đang ngủ trên condvar này */
	}

	pthread_mutex_destroy(&self->guard);
	return 0;
}

/* Các hàm hàng đợi chỉ được gọi khi đang giữ guard */
static void queue_push(struct condvar *self, struct condvar_waiter *w)
{
	w->next = NULL;
	if (self->tail) {
		self->tail->next = w;
	} else {
		self->head = w;
	}
	self->tail = w;
}

static struct condvar_waiter *queue_pop(struct condvar *self)
{
	struct condvar_waiter *w = self->head;

	if (w) {
		self->head = w->next;
		if (!self->head) {
			self->tail = NULL;
		}
	}
	return w;
}

static void queue_remove(struct condvar *self, struct condvar_waiter *w)
{
	struct condvar_waiter *prev = NULL;

	for (struct condvar_waiter *it = self->head; it; prev = it, it = it->next) {
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

int condvar_wait(struct condvar *self, struct mutex *mutex, int32_t timeout_ms)
{
	if (!self || !mutex || timeout_ms < MUTEX_FOREVER) {
		return -EINVAL;
	}

	/*
	 * Phải giữ mutex đúng một tầng: nếu đang khóa lồng, mutex_unlock bên dưới
	 * chỉ bớt một tầng, mutex vẫn bị giữ, và không ai đổi được điều kiện cho ta.
	 */
	int depth = mutex_lock_depth(mutex);
	if (depth < 0) {
		return -EPERM;
	}
	if (depth > 1) {
		return -EDEADLK;
	}

	struct condvar_waiter w = { .woken = false, .next = NULL };
	struct timespec deadline = deadline_after_ms(timeout_ms > 0 ? timeout_ms : 0);
	int ret = 0;

	pthread_cond_init(&w.wake, NULL);

	/*
	 * Vào hàng đợi TRƯỚC, nhả mutex SAU, và cả hai trong lúc giữ guard.
	 * Người signal cần guard, nên không thể chen vào giữa "kiểm tra điều kiện
	 * (lúc còn giữ mutex)" và "đã nằm trong hàng đợi": không mất tín hiệu.
	 */
	pthread_mutex_lock(&self->guard);
	queue_push(self, &w);
	mutex_unlock(mutex);

	while (!w.woken) {
		if (timeout_ms == MUTEX_FOREVER) {
			pthread_cond_wait(&w.wake, &self->guard);
		} else if (pthread_cond_timedwait(&w.wake, &self->guard, &deadline) == ETIMEDOUT &&
			   !w.woken) {
			queue_remove(self, &w);
			ret = -EAGAIN;
			break;
		}
	}

	pthread_mutex_unlock(&self->guard);
	pthread_cond_destroy(&w.wake);

	/*
	 * Như sách: lấy lại mutex với FOREVER, kể cả khi đã hết giờ, để caller
	 * luôn giữ mutex khi condvar_wait return. Timeout chỉ áp dụng cho việc chờ tín hiệu.
	 */
	mutex_lock(mutex, MUTEX_FOREVER);
	return ret;
}

int condvar_signal(struct condvar *self)
{
	if (!self) {
		return -EINVAL;
	}

	pthread_mutex_lock(&self->guard);
	struct condvar_waiter *w = queue_pop(self);
	if (w) {
		w->woken = true;
		pthread_cond_signal(&w->wake);
	}
	/* Không ai chờ thì tín hiệu mất luôn: condvar không có bộ nhớ như semaphore */
	pthread_mutex_unlock(&self->guard);
	return 0;
}

int condvar_broadcast(struct condvar *self)
{
	if (!self) {
		return -EINVAL;
	}

	int woken = 0;

	/* Lấy hết hàng đợi trong một lần giữ guard: mọi waiter cùng được đánh dấu sẵn sàng */
	pthread_mutex_lock(&self->guard);
	struct condvar_waiter *w;
	while ((w = queue_pop(self)) != NULL) {
		w->woken = true;
		pthread_cond_signal(&w->wake);
		woken++;
	}
	pthread_mutex_unlock(&self->guard);
	return woken;
}
