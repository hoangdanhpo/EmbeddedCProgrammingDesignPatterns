#include "uart.h"
#include "uart_hw.h"

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define UART_TX_BUF_SIZE 32

#ifndef UART_POOL_SIZE
#define UART_POOL_SIZE 2
#endif

/* Định nghĩa thật của struct: chỉ code trong file này truy cập được field */
struct uart {
	const char *name;
	uint32_t baudrate;
	struct uart_hw_regs regs; /* Kiểu từ header vendor, không lọt ra uart.h */
	uint8_t tx_buf[UART_TX_BUF_SIZE];
	size_t tx_len;
};

size_t uart_size(void)
{
	return sizeof(struct uart);
}

struct uart *uart_new(void)
{
	return malloc(sizeof(struct uart));
}

void uart_free(struct uart **self)
{
	if (!self) {
		return;
	}

	free(*self);
	/* Xóa con trỏ của caller để lỗi dùng-sau-khi-free lộ ra ngay thay vì ghi đè heap */
	*self = NULL;
}

/*
 * Pool là state của bộ cấp phát, không phải state của object: mỗi phần tử
 * vẫn là một object độc lập. Chưa an toàn khi gọi từ nhiều thread (bài 11-13).
 */
static struct uart uart_pool[UART_POOL_SIZE];
static bool uart_pool_used[UART_POOL_SIZE];

struct uart *uart_pool_get(void)
{
	for (size_t i = 0; i < UART_POOL_SIZE; i++) {
		if (!uart_pool_used[i]) {
			uart_pool_used[i] = true;
			return &uart_pool[i];
		}
	}
	return NULL;
}

void uart_pool_put(struct uart **self)
{
	if (!self || !*self) {
		return;
	}

	for (size_t i = 0; i < UART_POOL_SIZE; i++) {
		if (&uart_pool[i] == *self) {
			uart_pool_used[i] = false;
			break;
		}
	}
	*self = NULL;
}

int uart_init(struct uart *self, const char *name, uint32_t baudrate)
{
	if (!self || !name || baudrate == 0) {
		return -EINVAL;
	}

	memset(self, 0, sizeof(*self));
	self->name = name;
	self->baudrate = baudrate;
	self->regs.BRR = UART_HW_CLOCK_HZ / baudrate;
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

	printf("[%s @ %lu, BRR=%lu] %.*s\n", self->name, (unsigned long)self->baudrate,
	       (unsigned long)self->regs.BRR, (int)self->tx_len, (const char *)self->tx_buf);
	self->tx_len = 0;
	return 0;
}

size_t uart_tx_pending(const struct uart *self)
{
	return self ? self->tx_len : 0;
}
