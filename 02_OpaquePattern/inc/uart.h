#ifndef UART_H
#define UART_H

#include <stddef.h>
#include <stdint.h>

/*
 * Chỉ khai báo, không định nghĩa. Bên ngoài uart.c không ai biết struct có
 * những field gì hay lớn bao nhiêu, nên chỉ có thể cầm con trỏ tới nó.
 */
struct uart;

/* Cấp phát kiểu 1, trên stack: caller dùng alloca(uart_size()) */
size_t uart_size(void);

/* Cấp phát kiểu 2, trên heap: bắt buộc gọi uart_free() khi xong */
struct uart *uart_new(void);
void uart_free(struct uart **self);

/* Cấp phát kiểu 3, từ pool tĩnh bên trong uart.c: trả NULL khi pool đã hết */
struct uart *uart_pool_get(void);
void uart_pool_put(struct uart **self);

int uart_init(struct uart *self, const char *name, uint32_t baudrate);
int uart_deinit(struct uart *self);
int uart_write(struct uart *self, const uint8_t *data, size_t len);
int uart_flush(struct uart *self);

/* Caller không đọc được field tx_len nữa, nên cần getter */
size_t uart_tx_pending(const struct uart *self);

#endif /* UART_H */
