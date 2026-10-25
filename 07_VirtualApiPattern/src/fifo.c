#include "fifo.h"

#include <string.h>

void fifo_init(struct fifo *self)
{
	memset(self, 0, sizeof(*self));
}

size_t fifo_put(struct fifo *self, const uint8_t *data, size_t len)
{
	size_t room = FIFO_SIZE - self->count;
	size_t n = len < room ? len : room;

	/* memcpy với con trỏ NULL là undefined behavior kể cả khi n = 0 */
	if (n == 0) {
		return 0;
	}
	memcpy(&self->buf[self->count], data, n);
	self->count += n;
	return n;
}

size_t fifo_get(struct fifo *self, uint8_t *data, size_t len)
{
	size_t n = len < self->count ? len : self->count;

	if (n == 0) {
		return 0;
	}
	memcpy(data, self->buf, n);
	/* Dồn phần còn lại về đầu: đơn giản, đủ cho buffer 32 byte của demo */
	memmove(self->buf, &self->buf[n], self->count - n);
	self->count -= n;
	return n;
}
