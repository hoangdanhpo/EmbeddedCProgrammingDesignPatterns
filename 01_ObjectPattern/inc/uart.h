#ifndef UART_H
#define UART_H

#include <stddef.h>
#include <stdint.h>

#define UART_TX_BUF_SIZE 32

/*
 * Toàn bộ state của một UART nằm trong struct này, không có biến static
 * nào trong uart.c. Nhờ vậy có thể tạo bao nhiêu instance cũng được.
 */
struct uart {
	const char *name; /* Trên phần cứng thật: địa chỉ base của thanh ghi peripheral */
	uint32_t baudrate;
	uint8_t tx_buf[UART_TX_BUF_SIZE];
	size_t tx_len;
};

int uart_init(struct uart *self, const char *name, uint32_t baudrate);
int uart_deinit(struct uart *self);
int uart_write(struct uart *self, const uint8_t *data, size_t len);
int uart_flush(struct uart *self);

#endif /* UART_H */
