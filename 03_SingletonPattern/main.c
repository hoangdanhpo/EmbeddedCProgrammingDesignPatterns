#include "crc.h"
#include "logger.h"
#include "sys_init.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

/* Sink in ra stdout. Trên board thật: một sink ghi UART, một sink ghi RTT/flash... */
static void console_sink_write(struct logger_sink *self, const char *line)
{
	printf("%-7s| %s\n", self->name, line);
}

/*
 * Sink là object của caller: static trong main.c để sống suốt chương trình,
 * vì logger giữ con trỏ tới nó mãi mãi (không có hàm gỡ đăng ký).
 */
static struct logger_sink console_sink = { .name = "console", .write = console_sink_write };
static struct logger_sink uart1_sink = { .name = "uart1", .write = console_sink_write };

/* Cấu hình board chạy TRƯỚC main(), sau logger (priority lớn hơn) */
static int board_sys_init(void)
{
	logger_add_sink(&console_sink);
	return logger_write(LOGGER_LEVEL_INFO, "board", "sinks ready (before main)");
}

SYS_INIT(board_sys_init, SYS_INIT_PRIO_BOARD)

static void demo_logger(void)
{
	struct logger_stats stats;

	logger_write(LOGGER_LEVEL_DEBUG, "app", "hidden: below default level INFO");
	logger_set_level(LOGGER_LEVEL_DEBUG);
	logger_write(LOGGER_LEVEL_DEBUG, "app", "visible after set_level(DEBUG)");

	logger_add_sink(&uart1_sink);
	printf("add uart1 again -> %d (-EALREADY = %d)\n", logger_add_sink(&uart1_sink), -EALREADY);
	logger_write(LOGGER_LEVEL_WARN, "app", "goes to both sinks");

	logger_get_stats(&stats);
	printf("stats: written=%lu filtered=%lu\n", (unsigned long)stats.written,
	       (unsigned long)stats.filtered);
}

static void demo_crc(void)
{
	static const uint8_t check[] = "123456789";
	const size_t len = sizeof(check) - 1; /* bỏ ký tự '\0' cuối chuỗi */
	struct crc *a;
	struct crc *b;
	uint32_t value = 0;
	int ret;

	printf("crc init count before first use: %lu\n", (unsigned long)crc_init_count());
	crc_compute(check, len, &value);
	printf("crc_compute(\"123456789\") = 0x%08lX, init count = %lu\n", (unsigned long)value,
	       (unsigned long)crc_init_count());

	a = crc_acquire();
	b = crc_acquire();
	printf("acquire: a=%s b=%s\n", a ? "ok" : "NULL", b ? "ok" : "NULL");
	printf("crc_compute while a holds it -> %d (-EBUSY = %d)\n", crc_compute(check, len, &value),
	       -EBUSY);

	/* API stateful: reset -> feed -> feed -> result, chỉ an toàn vì a đang độc quyền */
	crc_reset(a);
	crc_feed(a, check, 4);
	crc_feed(a, check + 4, len - 4);
	crc_result(a, &value);
	printf("a: fed in two parts = 0x%08lX\n", (unsigned long)value);

	/*
	 * Gọi release trên một dòng riêng: thứ tự tính các tham số của printf là
	 * không xác định, nếu viết chung thì "a" có thể bị đọc TRƯỚC khi release chạy.
	 */
	ret = crc_release(&a);
	printf("release a -> %d, a is %s\n", ret, a ? "NOT NULL" : "NULL");
	ret = crc_release(&a);
	printf("release a again -> %d (-EINVAL = %d)\n", ret, -EINVAL);

	b = crc_acquire();
	printf("acquire b after release: %s, init count = %lu\n", b ? "ok" : "NULL",
	       (unsigned long)crc_init_count());
	crc_release(&b);
}

int main(void)
{
	printf("main() starts\n");
	demo_logger();
	demo_crc();
	return 0;
}
