#include "sys_events.h"

#include <errno.h>
#include <time.h>

int sys_events_init(struct sys_events *self)
{
	if (!self) {
		return -EINVAL;
	}

	self->bits = 0;
	int ret = mutex_init(&self->lock);
	if (ret) {
		return ret;
	}
	ret = condvar_init(&self->changed);
	if (ret) {
		mutex_deinit(&self->lock);
	}
	return ret;
}

int sys_events_deinit(struct sys_events *self)
{
	if (!self) {
		return -EINVAL;
	}

	int ret = condvar_deinit(&self->changed);
	if (ret) {
		return ret;
	}
	return mutex_deinit(&self->lock);
}

int sys_events_set(struct sys_events *self, uint32_t bits)
{
	if (!self) {
		return -EINVAL;
	}

	/* Đổi điều kiện KHI ĐANG GIỮ mutex, rồi mới báo: waiter không thể lỡ thay đổi này */
	mutex_lock(&self->lock, MUTEX_FOREVER);
	self->bits |= bits;
	int woken = condvar_broadcast(&self->changed);
	mutex_unlock(&self->lock);
	return woken;
}

static int64_t now_ms(void)
{
	struct timespec ts;

	timespec_get(&ts, TIME_UTC);
	return (int64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

int sys_events_wait_all(struct sys_events *self, uint32_t mask, int32_t timeout_ms, uint32_t *wakeups)
{
	if (!self || mask == 0 || timeout_ms < MUTEX_FOREVER) {
		return -EINVAL;
	}

	/* Hạn chót tính MỘT lần: bị đánh thức "nhầm" nhiều lần cũng không kéo dài tổng thời gian chờ */
	int64_t deadline = now_ms() + (timeout_ms > 0 ? timeout_ms : 0);
	uint32_t count = 0;
	int ret = 0;

	mutex_lock(&self->lock, MUTEX_FOREVER);
	while ((self->bits & mask) != mask) {
		int32_t remaining = MUTEX_FOREVER;
		if (timeout_ms != MUTEX_FOREVER) {
			int64_t left = deadline - now_ms();
			if (left <= 0) {
				ret = -EAGAIN;
				break;
			}
			remaining = (int32_t)left;
		}
		/* Return 0 chỉ nghĩa là "có thay đổi": quay lại đầu vòng để kiểm tra đúng điều kiện của mình */
		condvar_wait(&self->changed, &self->lock, remaining);
		count++;
	}
	mutex_unlock(&self->lock);

	if (wakeups) {
		*wakeups = count;
	}
	return ret;
}
