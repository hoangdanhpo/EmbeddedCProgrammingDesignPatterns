#include "buf_queue.h"

#include <errno.h>
#include <string.h>

int buf_queue_init(struct buf_queue *self)
{
	if (!self) {
		return -EINVAL;
	}

	memset(self, 0, sizeof(*self));
	return sem_init(&self->ready, 0, BUF_QUEUE_LEN);
}

int buf_queue_put(struct buf_queue *self, struct rx_buf *buf)
{
	if (!self) {
		return -EINVAL;
	}

	struct spinlock_key key = spinlock_lock(&self->lock);
	if (self->count == BUF_QUEUE_LEN) {
		spinlock_unlock(&self->lock, key);
		return -ENOSPC;
	}
	self->items[self->head] = buf;
	self->head = (self->head + 1) % BUF_QUEUE_LEN;
	self->count++;
	spinlock_unlock(&self->lock, key);

	/* Dữ liệu đã nằm yên trong ring rồi mới báo: worker thức dậy chắc chắn
	 * lấy được phần tử */
	return sem_give(&self->ready);
}

int buf_queue_get(struct buf_queue *self, struct rx_buf **out, int32_t timeout_ms)
{
	if (!self || !out) {
		return -EINVAL;
	}

	/* Mỗi lần take thành công ứng với đúng một phần tử đã được put */
	int ret = sem_take(&self->ready, timeout_ms);
	if (ret) {
		return ret;
	}

	struct spinlock_key key = spinlock_lock(&self->lock);
	*out = self->items[self->tail];
	self->tail = (self->tail + 1) % BUF_QUEUE_LEN;
	self->count--;
	spinlock_unlock(&self->lock, key);
	return 0;
}
