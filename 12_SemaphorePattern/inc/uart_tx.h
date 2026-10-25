#ifndef UART_TX_H
#define UART_TX_H

#include "sem.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define UART_TX_FIFO_SIZE 16
#define UART_TX_IRQ_CPU 7 /* CPU giả lập xử lý ngắt "TX complete" */

/*
 * Ví dụ some_device_transmit() của sách: nạp FIFO, rồi NGỦ trên semaphore cho
 * tới khi ngắt "gửi xong" của phần cứng give. Trong lúc chờ, CPU rảnh cho
 * thread khác (hoặc vào chế độ tiết kiệm điện), không có vòng polling nào.
 */
struct uart_tx {
	const char *name;
	uint32_t byte_time_us;            /* thời gian đẩy 1 byte ra dây: 10 bit / baud */
	uint8_t fifo[UART_TX_FIFO_SIZE];  /* "FIFO phần cứng" giả lập */
	size_t fifo_len;
	int status;                       /* ISR ghi: 0 = gửi xong, -EIO = lỗi đường truyền */
	bool fault;                       /* demo: lần gửi tới sẽ lỗi */
	struct sem done;                  /* ISR give khi phần cứng xong (dù thành công hay lỗi) */
};

int uart_tx_init(struct uart_tx *self, const char *name, uint32_t baudrate);

/*
 * Block tới khi phần cứng gửi xong. Trả 0, -EIO (phần cứng báo lỗi),
 * -EAGAIN (hết timeout_ms mà phần cứng không trả lời), -ENOSPC (quá FIFO).
 */
int uart_tx_transmit(struct uart_tx *self, const uint8_t *data, size_t len, int32_t timeout_ms);

/* Chỉ dùng cho demo: làm cho lần gửi tới bị lỗi đường truyền */
void uart_tx_inject_fault(struct uart_tx *self);

#endif /* UART_TX_H */
