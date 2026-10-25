#include "sample_queue.h"

#include <errno.h>
#include <time.h>

int sample_queue_init(struct sample_queue *self)
{
	if (!self) {
		return -EINVAL;
	}

	self->head = 0;
	self->count = 0;
	self->closed = false;
	int ret = mutex_init(&self->lock);
	if (ret) {
		return ret;
	}
	ret = condvar_init(&self->not_empty);
	if (ret) {
		mutex_deinit(&self->lock);
		return ret;
	}
	ret = condvar_init(&self->not_full);
	if (ret) {
		condvar_deinit(&self->not_empty);
		mutex_deinit(&self->lock);
	}
	return ret;
}

int sample_queue_deinit(struct sample_queue *self)
{
	if (!self) {
		return -EINVAL;
	}

	int ret = condvar_deinit(&self->not_full);
	if (ret == 0) {
		ret = condvar_deinit(&self->not_empty);
	}
	if (ret == 0) {
		ret = mutex_deinit(&self->lock);
	}
	return ret;
}

static int64_t now_ms(void)
{
	struct timespec ts;

	timespec_get(&ts, TIME_UTC);
	return (int64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

/* Thời gian chờ còn lại cho lần condvar_wait tiếp theo; -1 (FOREVER) giữ nguyên, 0 = đã hết */
static int32_t remaining_ms(int32_t timeout_ms, int64_t deadline)
{
	if (timeout_ms == MUTEX_FOREVER) {
		return MUTEX_FOREVER;
	}
	int64_t left = deadline - now_ms();
	return left > 0 ? (int32_t)left : 0;
}

int sample_queue_put(struct sample_queue *self, int32_t value, int32_t timeout_ms)
{
	if (!self || timeout_ms < MUTEX_FOREVER) {
		return -EINVAL;
	}

	int64_t deadline = now_ms() + (timeout_ms > 0 ? timeout_ms : 0);
	int ret = 0;

	mutex_lock(&self->lock, MUTEX_FOREVER);
	while (self->count == SAMPLE_QUEUE_CAP && !self->closed) {
		int32_t left = remaining_ms(timeout_ms, deadline);
		if (left == 0) {
			ret = -EAGAIN;
			break;
		}
		condvar_wait(&self->not_full, &self->lock, left);
	}
	if (ret == 0 && self->closed) {
		ret = -EPIPE;
	}
	if (ret == 0) {
		self->buf[(self->head + self->count) % SAMPLE_QUEUE_CAP] = value;
		self->count++;
		/* Một mẫu mới chỉ nuôi được MỘT consumer: signal là đủ, broadcast chỉ đánh thức thừa */
		condvar_signal(&self->not_empty);
	}
	mutex_unlock(&self->lock);
	return ret;
}

int sample_queue_get(struct sample_queue *self, int32_t *value, int32_t timeout_ms)
{
	if (!self || !value || timeout_ms < MUTEX_FOREVER) {
		return -EINVAL;
	}

	int64_t deadline = now_ms() + (timeout_ms > 0 ? timeout_ms : 0);
	int ret = 0;

	mutex_lock(&self->lock, MUTEX_FOREVER);
	while (self->count == 0 && !self->closed) {
		int32_t left = remaining_ms(timeout_ms, deadline);
		if (left == 0) {
			ret = -EAGAIN;
			break;
		}
		condvar_wait(&self->not_empty, &self->lock, left);
	}
	if (ret == 0 && self->count == 0) {
		ret = -EPIPE; /* Đã đóng và không còn mẫu nào */
	}
	if (ret == 0) {
		*value = self->buf[self->head];
		self->head = (self->head + 1) % SAMPLE_QUEUE_CAP;
		self->count--;
		condvar_signal(&self->not_full);
	}
	mutex_unlock(&self->lock);
	return ret;
}

int sample_queue_close(struct sample_queue *self)
{
	if (!self) {
		return -EINVAL;
	}

	mutex_lock(&self->lock, MUTEX_FOREVER);
	self->closed = true;
	/* Điều kiện "closed" liên quan tới MỌI thread đang chờ, nên phải broadcast */
	condvar_broadcast(&self->not_empty);
	condvar_broadcast(&self->not_full);
	mutex_unlock(&self->lock);
	return 0;
}
