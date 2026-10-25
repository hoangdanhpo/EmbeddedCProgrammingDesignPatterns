#ifndef SENSOR_FACTORY_H
#define SENSOR_FACTORY_H

#include <stddef.h>
#include <stdint.h>

#include "sensor.h"

/*
 * Factory: tạo cảm biến từ một bản mô tả dữ liệu (struct sensor_config).
 * Caller không biết struct tmp102, struct lm35 tồn tại; nó chỉ nhận về sensor_t.
 *
 * Bộ nhớ lấy từ pool tĩnh bên trong factory (không malloc), nên factory là
 * một singleton: không có self, pool là static trong sensor_factory.c.
 */

enum sensor_type {
	SENSOR_TYPE_TMP102, /* cảm biến số, I2C, 0.0625 °C/LSB */
	SENSOR_TYPE_LM35,   /* cảm biến tương tự 10 mV/°C, đọc qua ADC 12 bit */
	SENSOR_TYPE_FAKE,   /* trả giá trị cố định: dùng khi bring-up board hoặc test */
};

struct sensor_config {
	enum sensor_type type;
	const char *name;
	/* Mỗi loại cần tham số khác nhau; "type" cho biết phần nào của union có nghĩa */
	union {
		struct {
			uint8_t i2c_addr;
		} tmp102;
		struct {
			uint8_t adc_channel;
			uint16_t vref_mv;
		} lm35;
		struct {
			int32_t value_milli_c;
		} fake;
	} params;
};

/*
 * Trả 0 và ghi handle vào *out; hoặc mã lỗi âm:
 *  -EINVAL tham số sai, -ENOMEM hết slot trong pool, -ENODEV không thấy phần cứng.
 */
int sensor_create(const struct sensor_config *cfg, sensor_t *out);

/* Gọi cleanup của object, trả slot về pool, đặt handle của caller về NULL */
int sensor_destroy(sensor_t *sensor);

size_t sensor_factory_free_slots(void);

#endif /* SENSOR_FACTORY_H */
