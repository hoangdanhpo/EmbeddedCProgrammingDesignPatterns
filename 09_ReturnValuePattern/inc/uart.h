#ifndef UART_H
#define UART_H

#include <stddef.h>
#include <stdint.h>

#include "must_check.h"

#define UART_TX_BUF_SIZE 24

struct uart {
	const char *name;
	uint32_t baudrate;
	uint8_t tx_buf[UART_TX_BUF_SIZE];
	size_t tx_len;
};

/* Trả về 0 hoặc -EINVAL */
MUST_CHECK int uart_init(struct uart *self, const char *name, uint32_t baudrate);

/*
 * Chép tối đa len byte vào buffer gửi. Giống write() của POSIX, có thể chỉ
 * nhận một phần.
 *
 * Trả về:
 *   > 0      số byte đã nhận (có thể nhỏ hơn len: thành công một phần)
 *   0        chỉ khi len == 0
 *   -EINVAL  tham số sai
 *   -EAGAIN  buffer đầy, không nhận được byte nào; flush rồi thử lại
 */
MUST_CHECK int uart_write(struct uart *self, const uint8_t *data, size_t len);

/* Gửi hết buffer ra "đường dây" (stdout). Trả về 0 hoặc -EINVAL */
MUST_CHECK int uart_flush(struct uart *self);

/*
 * Số byte đang chờ gửi, trả qua out_len. Bản sửa của getter ở bài 02:
 * self == NULL giờ trả -EINVAL, không còn lẫn với "0 byte đang chờ".
 */
MUST_CHECK int uart_tx_pending(const struct uart *self, size_t *out_len);

#endif /* UART_H */
