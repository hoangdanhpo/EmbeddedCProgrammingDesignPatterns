#ifndef TEMP_SENSOR_H
#define TEMP_SENSOR_H

#include <stdbool.h>
#include <stdint.h>

#include "i2c_bus.h"
#include "must_check.h"

/* Bản đồ thanh ghi phỏng theo cảm biến nhiệt TMP117 (đơn giản hóa) */
#define TEMP_SENSOR_REG_TEMP       0x00
#define TEMP_SENSOR_REG_CONFIG     0x01
#define TEMP_SENSOR_REG_DEVICE_ID  0x0F
#define TEMP_SENSOR_DEVICE_ID      0x0117
#define TEMP_SENSOR_CFG_DATA_READY (1u << 13)

struct temp_sensor {
	struct i2c_bus *bus;
	uint8_t addr;
	bool ready;
};

/*
 * Trả về:
 *   0        thành công
 *   -EINVAL  tham số sai
 *   -ENODEV  không có cảm biến đúng loại ở địa chỉ addr
 *            (không ai ACK, hoặc thanh ghi ID sai)
 *   -EIO     lỗi bus khác khi đọc ID
 */
MUST_CHECK int temp_sensor_init(struct temp_sensor *self, struct i2c_bus *bus, uint8_t addr);

/*
 * Đọc nhiệt độ, đơn vị mili-độ C. Kết quả đi qua con trỏ out_mdeg_c,
 * giá trị trả về chỉ dành cho trạng thái.
 *
 * Trả về:
 *   0        thành công, *out_mdeg_c hợp lệ
 *   -EINVAL  tham số sai hoặc chưa init
 *   -EAGAIN  cảm biến chưa đo xong, gọi lại sau
 *   -EIO     lỗi giao tiếp I2C (bất kể bus báo lỗi gì bên dưới)
 *
 * Khi trả lỗi, *out_mdeg_c không bị thay đổi.
 */
MUST_CHECK int temp_sensor_read(struct temp_sensor *self, int32_t *out_mdeg_c);

#endif /* TEMP_SENSOR_H */
