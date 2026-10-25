#include "gps.h"
#include "mem_serial.h"
#include "serial.h"
#include "uart.h"
#include "usb_cdc.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

/*
 * Cấp phát tĩnh mọi object ở một chỗ (Object Pattern): RAM biết lúc build.
 * main.c là nơi "nối dây": nó biết các kiểu cụ thể, phần còn lại thì không.
 */
static struct uart uart1;
static struct usb_cdc usb;
static struct mem_serial mem;
static struct gps gps;

static int write_str(serial_t port, const char *str)
{
	return serial_write(port, (const uint8_t *)str, strlen(str));
}

/* Hàm này chỉ biết serial_t: cùng một code chạy cho mọi loại cổng */
static void broadcast(serial_t *ports, size_t count, const char *msg)
{
	for (size_t i = 0; i < count; i++) {
		int ret = write_str(ports[i], msg);

		printf("  port %u -> %d%s\n", (unsigned)i, ret, ret == -ENOTCONN ? " (-ENOTCONN)" : "");
	}
}

static void print_mem_tx(void)
{
	uint8_t buf[FIFO_SIZE + 1];
	size_t n = mem_serial_take_tx(&mem, buf, FIFO_SIZE);

	buf[n] = '\0';
	printf("  mem captured %u bytes: \"%.*s\"\n", (unsigned)n, (int)(n >= 2 ? n - 2 : n),
	       (const char *)buf);
}

int main(void)
{
	serial_t ports[3];
	uint8_t rx[16];
	int ret;

	uart_init(&uart1, "UART1", 115200);
	usb_cdc_init(&usb, 0x81);
	mem_serial_init(&mem);

	ports[0] = uart_to_serial(&uart1);
	ports[1] = usb_cdc_to_serial(&usb);
	ports[2] = mem_serial_to_serial(&mem);

	printf("1) broadcast, USB not connected yet:\n");
	broadcast(ports, 3, "ping\r\n");
	print_mem_tx();

	printf("2) broadcast after host opens the USB port:\n");
	usb_cdc_set_connected(&usb, true);
	broadcast(ports, 3, "ping\r\n");
	print_mem_tx();

	printf("3) gps.c does not know which port it uses:\n");
	gps_init(&gps, ports[0]);
	gps_send_cmd(&gps, "PMTK220,1000");
	gps_init(&gps, ports[1]);
	gps_send_cmd(&gps, "PMTK220,1000");
	gps_init(&gps, ports[2]);
	gps_send_cmd(&gps, "PMTK220,1000");
	print_mem_tx();

	printf("4) read through the same interface:\n");
	uart_isr_rx_byte(&uart1, 'O');
	uart_isr_rx_byte(&uart1, 'K');
	usb_cdc_host_send(&usb, (const uint8_t *)"hi", 2);
	for (size_t i = 0; i < 3; i++) {
		ret = serial_read(ports[i], rx, sizeof(rx));
		printf("  port %u read -> %d \"%.*s\"\n", (unsigned)i, ret, ret > 0 ? ret : 0,
		       (const char *)rx);
	}
	return 0;
}
