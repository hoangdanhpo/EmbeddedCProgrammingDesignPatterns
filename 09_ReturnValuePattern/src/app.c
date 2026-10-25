#include "app.h"
#include "errstr.h"

#include <errno.h>
#include <stdio.h>

int app_init(struct app *self, struct temp_sensor *sensor, struct uart *console)
{
	if (!self || !sensor || !console) {
		return -EINVAL;
	}

	self->sensor = sensor;
	self->console = console;
	self->io_errors = 0;
	return 0;
}

int app_console_write_all(struct app *self, const char *str, size_t len)
{
	if (!self || (len && !str)) {
		return -EINVAL;
	}

	size_t done = 0;

	while (done < len) {
		int ret = uart_write(self->console, (const uint8_t *)str + done, len - done);

		if (ret == -EAGAIN) {
			/* Buffer đầy: xử lý được ngay tại đây bằng cách đẩy dữ liệu ra rồi thử lại */
			if (uart_flush(self->console) < 0) {
				return -EIO;
			}
			continue;
		}
		if (ret < 0) {
			return -EIO;
		}
		/* ret > 0: thành công một phần, tiến lên đúng số byte đã được nhận */
		done += (size_t)ret;
	}

	return uart_flush(self->console) < 0 ? -EIO : 0;
}

int app_report_temperature(struct app *self)
{
	int32_t mdeg = 0;
	int ret = -EAGAIN;
	char line[32];

	if (!self) {
		return -EINVAL;
	}

	for (int attempt = 1; attempt <= APP_READ_RETRIES && ret == -EAGAIN; attempt++) {
		ret = temp_sensor_read(self->sensor, &mdeg);
		if (ret == -EAGAIN) {
			printf("  app: attempt %d -> %s, retry\n", attempt, errstr(ret));
		}
	}

	if (ret == -EAGAIN) {
		/* Hết số lần thử: đổi sang mã mô tả đúng trạng thái của *hàm này* */
		return -ETIMEDOUT;
	}
	if (ret < 0) {
		/* Ghi log VÀ trả mã lỗi: log cho con người, mã lỗi cho code phía trên */
		self->io_errors++;
		printf("  app: sensor read failed: %s (io_errors=%lu)\n", errstr(ret),
		       (unsigned long)self->io_errors);
		return -EIO;
	}

	/* Tách dấu riêng: trong C, -500 / 1000 bằng 0, in thẳng sẽ mất dấu trừ */
	const char *sign = mdeg < 0 ? "-" : "";
	int32_t abs_mdeg = mdeg < 0 ? -mdeg : mdeg;
	int n = snprintf(line, sizeof(line), "T=%s%ld.%03ld C\n", sign, (long)(abs_mdeg / 1000),
			 (long)(abs_mdeg % 1000));

	/* snprintf cũng theo quy ước "âm = lỗi, dương = số ký tự"; >= size nghĩa là bị cắt */
	if (n < 0 || (size_t)n >= sizeof(line)) {
		return -EIO;
	}

	return app_console_write_all(self, line, (size_t)n);
}
