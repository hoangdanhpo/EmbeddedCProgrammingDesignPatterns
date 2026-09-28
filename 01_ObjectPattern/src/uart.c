#include "uart.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

int uart_init(struct uart *self, const char *name, uint32_t baudrate)
{
	if (!self || !name || baudrate == 0) {
		return -EINVAL;
	}

	/* Xóa sạch trước khi dùng: object nằm trên stack sẽ chứa rác */
	memset(self, 0, sizeof(*self));
	self->name = name;
	self->baudrate = baudrate;
	return 0;
}

int uart_deinit(struct uart *self)
{
	if (!self) {
		return -EINVAL;
	}

	memset(self, 0, sizeof(*self));
	return 0;
}

int uart_write(struct uart *self, const uint8_t *data, size_t len)
{
	if (!self || !data) {
		return -EINVAL;
	}

	/* Từ chối cả gói thay vì ghi một nửa, để caller không phải xử lý trường hợp ghi thiếu */
	if (len > UART_TX_BUF_SIZE - self->tx_len) {
		return -ENOSPC;
	}

	memcpy(&self->tx_buf[self->tx_len], data, len);
	self->tx_len += len;
	return 0;
}

int uart_flush(struct uart *self)
{
	if (!self) {
		return -EINVAL;
	}

	/* Mô phỏng việc đẩy byte ra đường TX bằng printf */
	printf("[%s @ %lu] %.*s\n", self->name, (unsigned long)self->baudrate, (int)self->tx_len,
	       (const char *)self->tx_buf);
	self->tx_len = 0;
	return 0;
}
