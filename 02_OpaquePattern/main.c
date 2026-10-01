#include "uart.h"

#include <stdio.h>
#include <string.h>

/* alloca không thuộc chuẩn C nên mỗi nền tảng khai báo ở một header khác */
#ifdef _WIN32
#include <malloc.h>
#else
#include <alloca.h>
#endif

static int write_str(struct uart *uart, const char *str)
{
	return uart_write(uart, (const uint8_t *)str, strlen(str));
}

static void demo_stack(void)
{
	/* Bộ nhớ tự giải phóng khi hàm return, giống biến cục bộ thường */
	struct uart *uart = alloca(uart_size());

	uart_init(uart, "UART0", 115200);
	write_str(uart, "from stack");
	printf("pending before flush: %u bytes\n", (unsigned)uart_tx_pending(uart));
	uart_flush(uart);
	uart_deinit(uart);
}

static void demo_heap(void)
{
	struct uart *uart = uart_new();

	if (!uart) {
		printf("uart_new failed: out of heap\n");
		return;
	}

	uart_init(uart, "UART1", 9600);
	write_str(uart, "from heap");
	uart_flush(uart);
	uart_deinit(uart);
	uart_free(&uart);
	printf("after uart_free: uart is %s\n", uart ? "NOT NULL" : "NULL");
}

static void demo_pool(void)
{
	struct uart *a = uart_pool_get();
	struct uart *b = uart_pool_get();
	struct uart *c = uart_pool_get();

	printf("pool: a=%s b=%s c=%s\n", a ? "ok" : "NULL", b ? "ok" : "NULL", c ? "ok" : "NULL");

	uart_init(a, "UART2", 57600);
	write_str(a, "from pool");
	uart_flush(a);
	uart_deinit(a);
	uart_pool_put(&a);

	/* Slot của a vừa được trả lại nên lần này lấy được */
	c = uart_pool_get();
	printf("after put: c=%s\n", c ? "ok" : "NULL");

	uart_pool_put(&b);
	uart_pool_put(&c);
}

int main(void)
{
	printf("sizeof(struct uart) = %u bytes (main.c only knows it via uart_size())\n",
	       (unsigned)uart_size());

	demo_stack();
	demo_heap();
	demo_pool();
	return 0;
}
