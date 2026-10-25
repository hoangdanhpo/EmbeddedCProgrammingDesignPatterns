#include "rx_ring.h"

#include <errno.h>
#include <string.h>

int rx_ring_init(struct rx_ring *self)
{
	if (!self) {
		return -EINVAL;
	}

	memset(self, 0, sizeof(*self));
	return 0;
}

/*
 * Logic thật của ring, không tự lấy lock: caller phải giữ lock (hoặc là demo
 * cố tình không giữ). Đọc count một lần ở đầu, ghi lại ở cuối, đúng như cách
 * code embedded thường viết; khoảng giữa hai lệnh đó là chỗ race xảy ra.
 */
static int ring_put_raw(struct rx_ring *self, uint8_t byte)
{
	size_t n = self->count;

	if (n == RX_RING_SIZE) {
		self->overrun++;
		return -ENOSPC;
	}
	self->buf[self->head] = byte;
	self->head = (self->head + 1) % RX_RING_SIZE;
	self->count = n + 1;
	return 0;
}

static int ring_get_raw(struct rx_ring *self, uint8_t *out)
{
	size_t n = self->count;

	if (n == 0) {
		return -EAGAIN;
	}
	*out = self->buf[self->tail];
	self->tail = (self->tail + 1) % RX_RING_SIZE;
	self->count = n - 1;
	return 0;
}

int rx_ring_put(struct rx_ring *self, uint8_t byte)
{
	if (!self) {
		return -EINVAL;
	}

	/* Lock data, not code: vùng găng chỉ gồm vài lệnh đọc/ghi bộ nhớ */
	struct spinlock_key key = spinlock_lock(&self->lock);
	int ret = ring_put_raw(self, byte);
	spinlock_unlock(&self->lock, key);
	return ret;
}

int rx_ring_get(struct rx_ring *self, uint8_t *out)
{
	if (!self || !out) {
		return -EINVAL;
	}

	struct spinlock_key key = spinlock_lock(&self->lock);
	int ret = ring_get_raw(self, out);
	spinlock_unlock(&self->lock, key);
	return ret;
}

size_t rx_ring_count(struct rx_ring *self)
{
	if (!self) {
		return 0;
	}

	struct spinlock_key key = spinlock_lock(&self->lock);
	size_t n = self->count;
	spinlock_unlock(&self->lock, key);
	return n;
}

uint32_t rx_ring_overrun(struct rx_ring *self)
{
	if (!self) {
		return 0;
	}

	struct spinlock_key key = spinlock_lock(&self->lock);
	uint32_t n = self->overrun;
	spinlock_unlock(&self->lock, key);
	return n;
}

int rx_ring_put_nolock(struct rx_ring *self, uint8_t byte)
{
	return self ? ring_put_raw(self, byte) : -EINVAL;
}

int rx_ring_get_nolock(struct rx_ring *self, uint8_t *out)
{
	return (self && out) ? ring_get_raw(self, out) : -EINVAL;
}
