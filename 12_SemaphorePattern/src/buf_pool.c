#include "buf_pool.h"

#include <errno.h>
#include <string.h>

int buf_pool_init(struct buf_pool *self)
{
	if (!self) {
		return -EINVAL;
	}

	memset(self, 0, sizeof(*self));
	/* Ban đầu cả BUF_POOL_COUNT buffer đều rảnh */
	return sem_init(&self->free, BUF_POOL_COUNT, BUF_POOL_COUNT);
}

int buf_pool_get(struct buf_pool *self, struct rx_buf **out, int32_t timeout_ms)
{
	if (!self || !out) {
		return -EINVAL;
	}

	/* Bước 1: xin một "suất". Lấy được nghĩa là CHẮC CHẮN còn ít nhất một slot
	 * trống dành riêng cho mình, nên bước 2 không bao giờ thất bại. */
	int ret = sem_take(&self->free, timeout_ms);
	if (ret) {
		return ret;
	}

	/* Bước 2: chọn slot cụ thể. Nhiều thread/ISR có thể cùng tới đây, nên
	 * used[] phải nằm trong spinlock. Vùng găng ngắn, không gọi hàm nào. */
	struct spinlock_key key = spinlock_lock(&self->lock);
	for (size_t i = 0; i < BUF_POOL_COUNT; i++) {
		if (!self->used[i]) {
			self->used[i] = true;
			self->bufs[i].len = 0;
			*out = &self->bufs[i];
			break;
		}
	}
	self->in_use++;
	if (self->in_use > self->max_in_use) {
		self->max_in_use = self->in_use;
	}
	spinlock_unlock(&self->lock, key);
	return 0;
}

int buf_pool_put(struct buf_pool *self, struct rx_buf **buf)
{
	if (!self || !buf || !*buf) {
		return -EINVAL;
	}

	bool found = false;
	struct spinlock_key key = spinlock_lock(&self->lock);
	for (size_t i = 0; i < BUF_POOL_COUNT; i++) {
		if (&self->bufs[i] == *buf && self->used[i]) {
			self->used[i] = false;
			self->in_use--;
			found = true;
			break;
		}
	}
	spinlock_unlock(&self->lock, key);

	if (!found) {
		return -EINVAL; /* không thuộc pool, hoặc trả hai lần */
	}
	*buf = NULL;
	/* Give SAU khi slot đã thật sự trống: thread được đánh thức chắc chắn tìm
	 * thấy slot ở bước 2 của buf_pool_get */
	return sem_give(&self->free);
}

uint32_t buf_pool_max_in_use(struct buf_pool *self)
{
	if (!self) {
		return 0;
	}

	struct spinlock_key key = spinlock_lock(&self->lock);
	uint32_t n = self->max_in_use;
	spinlock_unlock(&self->lock, key);
	return n;
}
