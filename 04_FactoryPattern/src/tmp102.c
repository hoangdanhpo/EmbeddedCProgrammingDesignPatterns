#include "tmp102.h"
#include "board_sim.h"

#include <errno.h>
#include <string.h>

#define TMP102_REG_TEMP     0x00
#define TMP102_REG_CONFIG   0x01
#define TMP102_CONFIG_SD    0x0100 /* bit shutdown: tắt chip, dòng tiêu thụ còn ~0.5 uA */
#define TMP102_ADDR_MIN     0x48   /* chân ADD0 chọn được 4 địa chỉ 0x48..0x4B */
#define TMP102_ADDR_MAX     0x4B
#define TMP102_MILLI_C_X10  625    /* 1 LSB = 0.0625 °C = 62.5 m°C, nhân 10 để giữ số nguyên */

static int tmp102_read(sensor_t sensor, int32_t *out_milli_c)
{
	/* Từ handle (địa chỉ field base.api) lùi về đầu struct tmp102 */
	struct tmp102 *self = CONTAINER_OF(sensor, struct tmp102, base.api);
	uint16_t raw;
	int ret = board_i2c_read_u16(self->i2c_addr, TMP102_REG_TEMP, &raw);

	if (ret) {
		return ret;
	}

	/* 12 bit có dấu, căn trái: chia 16 (không dịch phải) để số âm cũng đúng chuẩn C */
	int32_t counts = (int16_t)raw / 16;
	*out_milli_c = counts * TMP102_MILLI_C_X10 / 10;
	return 0;
}

static void tmp102_deinit(sensor_t sensor)
{
	struct tmp102 *self = CONTAINER_OF(sensor, struct tmp102, base.api);
	uint16_t config;

	/* Đưa chip về chế độ ngủ khi không dùng nữa: việc dọn dẹp mà factory gọi hộ */
	if (board_i2c_read_u16(self->i2c_addr, TMP102_REG_CONFIG, &config) == 0) {
		board_i2c_write_u16(self->i2c_addr, TMP102_REG_CONFIG, config | TMP102_CONFIG_SD);
	}
}

static const struct sensor_api tmp102_api = {
	.read = tmp102_read,
	.deinit = tmp102_deinit,
};

int tmp102_init(struct tmp102 *self, const char *name, uint8_t i2c_addr)
{
	uint16_t config;

	if (!self || !name || i2c_addr < TMP102_ADDR_MIN || i2c_addr > TMP102_ADDR_MAX) {
		return -EINVAL;
	}

	memset(self, 0, sizeof(*self));
	self->base.api = &tmp102_api;
	self->base.name = name;
	self->i2c_addr = i2c_addr;

	/* Thử đọc một thanh ghi: phát hiện chip không được hàn ngay lúc tạo, không phải lúc đọc */
	if (board_i2c_read_u16(i2c_addr, TMP102_REG_CONFIG, &config) != 0) {
		return -ENODEV;
	}
	return 0;
}
