#include "uart.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

int uart_init(struct uart *self, const char *name, uint32_t baudrate)
{
	if (!self || !name || baudrate == 0) {
		return -EINVAL;
	}

	memset(self, 0, sizeof(*self));
	self->name = name;
	self->baudrate = baudrate;
	return 0;
}

int uart_write(struct uart *self, const uint8_t *data, size_t len)
{
	if (!self || (len && !data)) {
		return -EINVAL;
	}

	if (len == 0) {
		return 0;
	}

	size_t space = UART_TX_BUF_SIZE - self->tx_len;

	if (space == 0) {
		/* Không nhận được byte nào: phải là lỗi, vì trả 0 sẽ bị hiểu là "xong" */
		return -EAGAIN;
	}

	size_t n = len < space ? len : space;

	memcpy(&self->tx_buf[self->tx_len], data, n);
	self->tx_len += n;
	/* n <= UART_TX_BUF_SIZE nên ép sang int không bao giờ tràn */
	return (int)n;
}

int uart_flush(struct uart *self)
{
	if (!self) {
		return -EINVAL;
	}

	if (self->tx_len == 0) {
		return 0;
	}

	/* In trong ngoặc kép, đổi '\n' thành "\n" để thấy rõ ranh giới từng lần gửi */
	printf("  [%s tx %2u B] \"", self->name, (unsigned)self->tx_len);
	for (size_t i = 0; i < self->tx_len; i++) {
		if (self->tx_buf[i] == '\n') {
			printf("\\n");
		} else {
			putchar(self->tx_buf[i]);
		}
	}
	printf("\"\n");
	self->tx_len = 0;
	return 0;
}

int uart_tx_pending(const struct uart *self, size_t *out_len)
{
	if (!self || !out_len) {
		return -EINVAL;
	}

	*out_len = self->tx_len;
	return 0;
}
