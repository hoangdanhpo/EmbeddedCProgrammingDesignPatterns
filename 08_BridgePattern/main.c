#include "app.h"
#include "frame_logger.h"
#include "logger.h"
#include "mem_serial.h"
#include "serial.h"
#include "text_logger.h"
#include "uart.h"
#include "usb_cdc.h"

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>

/*
 * main.c là nơi duy nhất biết cả hai hierarchy và "bắc cầu" giữa chúng:
 * nó tạo transport (hierarchy serial), tạo logger (hierarchy logger) và đưa
 * handle serial_t vào logger. app.c chỉ thấy logger_t.
 */
static struct uart uart1;
static struct usb_cdc usb;
static struct mem_serial mem;

/* 2 loại logger x 3 transport = 6 tổ hợp, từ 2 + 3 implementation */
static struct text_logger text_log[3];
static struct frame_logger frame_log[3];
static struct text_logger console;

static void dump_mem(void)
{
	uint8_t buf[FIFO_SIZE];
	size_t n = mem_serial_take_tx(&mem, buf, sizeof(buf));

	printf("  [mem] captured %u bytes:", (unsigned)n);
	for (size_t i = 0; i < n; i++) {
		printf(" %02X", (unsigned)buf[i]);
	}
	printf("\n");
}

int main(void)
{
	static const char *const names[3] = { "uart", "usb", "mem" };
	serial_t transports[3];
	bool usb_plugged;
	int ret;

	uart_init(&uart1, "UART1", 115200);
	usb_cdc_init(&usb, 0x81);
	mem_serial_init(&mem);
	usb_cdc_set_connected(&usb, true);

	transports[0] = uart_to_serial(&uart1);
	transports[1] = usb_cdc_to_serial(&usb);
	transports[2] = mem_serial_to_serial(&mem);

	printf("1) 2 logger types x 3 transports, same app code:\n");
	for (size_t i = 0; i < 3; i++) {
		text_logger_init(&text_log[i], transports[i], LOG_DBG);
		frame_logger_init(&frame_log[i], transports[i]);

		printf(" text  -> %s\n", names[i]);
		app_report(text_logger_to_logger(&text_log[i]), 12, 23500);
		if (i == 2) {
			dump_mem();
		}
		printf(" frame -> %s\n", names[i]);
		app_report(frame_logger_to_logger(&frame_log[i]), 12, 23500);
		if (i == 2) {
			dump_mem();
		}
	}

	printf("2) pick the transport at runtime (USB unplugged -> UART):\n");
	usb_cdc_set_connected(&usb, false);
	usb_plugged = false; /* trên board thật: đọc chân VBUS */
	text_logger_init(&console, usb_plugged ? transports[1] : transports[0], LOG_INF);
	app_report(text_logger_to_logger(&console), 13, 71200);
	ret = logger_log(text_logger_to_logger(&console), LOG_DBG, "filtered out");
	printf("  LOG_DBG on an INF console -> %d (dropped by the logger, never crosses the bridge)\n",
	       ret);

	printf("3) an error on the far side reaches the app unchanged:\n");
	ret = app_report(frame_logger_to_logger(&frame_log[1]), 14, 23400);
	printf("  frame logger over unplugged USB -> %d%s, dropped=%lu\n", ret,
	       ret == -ENOTCONN ? " (-ENOTCONN)" : "",
	       (unsigned long)frame_logger_dropped(&frame_log[1]));
	return 0;
}
