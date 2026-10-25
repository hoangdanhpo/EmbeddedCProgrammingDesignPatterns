#ifndef APP_H
#define APP_H

#include <stdint.h>

#include "must_check.h"
#include "temp_sensor.h"
#include "uart.h"

#define APP_READ_RETRIES 3

struct app {
	struct temp_sensor *sensor;
	struct uart *console;
	uint32_t io_errors; /* đếm lỗi để chẩn đoán từ xa, không chỉ in log rồi quên */
};

/* Trả về 0 hoặc -EINVAL */
MUST_CHECK int app_init(struct app *self, struct temp_sensor *sensor, struct uart *console);

/*
 * Đọc nhiệt độ và in ra console.
 *
 * Trả về:
 *   0           thành công
 *   -EINVAL     tham số sai
 *   -ETIMEDOUT  cảm biến vẫn chưa có dữ liệu sau APP_READ_RETRIES lần thử
 *   -EIO        không đọc được cảm biến hoặc không gửi được ra console
 */
MUST_CHECK int app_report_temperature(struct app *self);

/*
 * Gửi trọn chuỗi ra UART, tự xử lý trường hợp uart_write chỉ nhận một phần.
 * Trả về 0, -EINVAL hoặc -EIO.
 */
MUST_CHECK int app_console_write_all(struct app *self, const char *str, size_t len);

#endif /* APP_H */
