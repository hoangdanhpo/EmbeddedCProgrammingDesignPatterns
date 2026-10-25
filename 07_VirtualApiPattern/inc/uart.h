#ifndef UART_H
#define UART_H

#include "fifo.h"
#include "serial.h"

#include <stdint.h>

/*
 * Implementation header của UART. Struct được định nghĩa ở đây (Object
 * Pattern) để board.c / main.c cấp phát tĩnh được. Field serial là con trỏ
 * tới struct serial_ops, kiểu mà file này chỉ biết qua khai báo trước trong
 * serial.h, nên người include uart.h vẫn không đọc hay gọi được bảng hàm.
 * Ngoài uart.c không ai đụng vào các field (quy ước của bài 1).
 */
struct uart {
	const struct serial_ops *serial; /* "vptr": trỏ tới bảng hàm const dùng chung */
	const char *name;
	uint32_t baudrate;
	struct fifo rx;                   /* byte nhận được từ ngắt RX, chờ serial_read */
};

int uart_init(struct uart *self, const char *name, uint32_t baudrate);
/* Đổi từ object cụ thể sang handle của interface serial */
serial_t uart_to_serial(struct uart *self);
/* Mô phỏng ngắt RX: phần cứng vừa nhận được một byte */
void uart_isr_rx_byte(struct uart *self, uint8_t byte);

#endif /* UART_H */
