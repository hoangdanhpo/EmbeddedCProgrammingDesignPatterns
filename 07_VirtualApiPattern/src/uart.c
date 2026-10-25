#include "uart.h"
#include "container_of.h"
#include "serial_ops.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

/*
 * Mô phỏng thanh ghi dữ liệu TX: trên chip thật đây là một lần ghi vào
 * USARTx->DR. Byte không in được hiện dưới dạng \xNN để thấy được dữ liệu nhị phân.
 */
static void uart_hw_tx_byte(uint8_t byte)
{
	if (byte == '\r') {
		printf("\\r");
	} else if (byte == '\n') {
		printf("\\n");
	} else if (isprint(byte)) {
		putchar(byte);
	} else {
		printf("\\x%02X", (unsigned)byte);
	}
}

/*
 * Các hàm của bảng ops luôn là static: chỉ được gọi qua bảng, không ai gọi
 * thẳng được, và tên không đụng với hàm cùng vai trò của driver khác.
 */
static int uart_serial_write(serial_t serial, const uint8_t *data, size_t len)
{
	/* serial trỏ vào field self->serial, lùi lại offset của field là ra self */
	struct uart *self = CONTAINER_OF(serial, struct uart, serial);

	printf("[%s @ %lu] tx: \"", self->name, (unsigned long)self->baudrate);
	for (size_t i = 0; i < len; i++) {
		uart_hw_tx_byte(data[i]);
	}
	printf("\"\n");
	/* UART gửi chặn từng byte nên luôn nhận đủ len byte */
	return (int)len;
}

static int uart_serial_read(serial_t serial, uint8_t *data, size_t len)
{
	struct uart *self = CONTAINER_OF(serial, struct uart, serial);

	return (int)fifo_get(&self->rx, data, len);
}

/* Một bảng duy nhất cho mọi instance UART, nằm trong Flash vì là const */
static const struct serial_ops uart_serial_ops = {
	.write = uart_serial_write,
	.read = uart_serial_read,
};

int uart_init(struct uart *self, const char *name, uint32_t baudrate)
{
	if (!self || !name || baudrate == 0) {
		return -EINVAL;
	}

	memset(self, 0, sizeof(*self));
	self->serial = &uart_serial_ops;
	self->name = name;
	self->baudrate = baudrate;
	fifo_init(&self->rx);
	return 0;
}

serial_t uart_to_serial(struct uart *self)
{
	return self ? &self->serial : NULL;
}

void uart_isr_rx_byte(struct uart *self, uint8_t byte)
{
	fifo_put(&self->rx, &byte, 1);
}
