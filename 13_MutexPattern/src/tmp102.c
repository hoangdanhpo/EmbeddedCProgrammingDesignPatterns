#include "tmp102.h"

#include <errno.h>

#define TMP102_REG_TEMP 0x00
#define TMP102_REG_CONFIG 0x01

int tmp102_init(struct tmp102 *self, struct i2c_bus *bus, uint8_t addr)
{
	if (!self || !bus) {
		return -EINVAL;
	}

	self->bus = bus;
	self->addr = addr;
	return 0;
}

int tmp102_read_mc(struct tmp102 *self, int32_t *milli_c)
{
	if (!self || !milli_c) {
		return -EINVAL;
	}

	uint8_t raw[2];
	int ret = i2c_bus_write_read(self->bus, self->addr, TMP102_REG_TEMP, raw, sizeof(raw));
	if (ret) {
		return ret;
	}

	/*
	 * 12 bit có dấu, căn trái trong 16 bit, 0.0625 độ/LSB.
	 * Chia 16 thay vì >> 4: dịch phải số âm là implementation-defined trong C,
	 * còn 4 bit thấp luôn bằng 0 nên phép chia cho kết quả chính xác.
	 */
	int16_t value = (int16_t)(((uint16_t)raw[0] << 8) | raw[1]);
	*milli_c = (int32_t)(value / 16) * 625 / 10;
	return 0;
}

int tmp102_read_snapshot(struct tmp102 *self, struct tmp102_snapshot *out)
{
	if (!self || !out) {
		return -EINVAL;
	}

	/* Giữ bus qua cả hai transfer; mỗi transfer bên trong lại khóa lồng thêm một lần */
	int ret = i2c_bus_lock(self->bus, MUTEX_FOREVER);
	if (ret) {
		return ret;
	}

	uint8_t cfg[2];
	ret = i2c_bus_write_read(self->bus, self->addr, TMP102_REG_CONFIG, cfg, sizeof(cfg));
	if (ret == 0) {
		out->config = (uint16_t)(((uint16_t)cfg[0] << 8) | cfg[1]);
		ret = tmp102_read_mc(self, &out->milli_c);
	}

	i2c_bus_unlock(self->bus);
	return ret;
}
