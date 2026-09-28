#include "uart.h"

#include <stdio.h>
#include <string.h>

static int write_str(struct uart *uart, const char *str)
{
	return uart_write(uart, (const uint8_t *)str, strlen(str));
}

int main(void)
{
	/* Hai instance độc lập, dùng chung một bộ code driver */
	struct uart debug_uart;
	struct uart modem_uart;

	if (uart_init(&debug_uart, "UART0", 115200) < 0 || uart_init(&modem_uart, "UART1", 9600) < 0) {
		printf("uart_init failed\n");
		return 1;
	}

	write_str(&debug_uart, "boot ok");
	write_str(&modem_uart, "AT+CSQ");

	/* Buffer của UART0 không ảnh hưởng gì tới UART1 */
	uart_flush(&debug_uart);
	uart_flush(&modem_uart);

	/* Gói lớn hơn buffer còn trống bị từ chối, trả về mã lỗi âm */
	int ret = write_str(&debug_uart, "this message is way too long for 32 bytes");
	printf("write oversized message -> %d\n", ret);

	uart_deinit(&debug_uart);
	uart_deinit(&modem_uart);
	return 0;
}
