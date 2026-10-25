#include "temp_sensor.h"

#include <errno.h>

/* Đọc một thanh ghi 16-bit. Hàm private nên được trả thẳng mã lỗi của bus. */
static int read_reg16(struct temp_sensor *self, uint8_t reg, uint16_t *out)
{
	uint8_t raw[2];
	int ret = i2c_bus_write_read(self->bus, self->addr, &reg, 1, raw, sizeof(raw));

	if (ret < 0) {
		return ret;
	}

	*out = (uint16_t)((raw[0] << 8) | raw[1]);
	return 0;
}

int temp_sensor_init(struct temp_sensor *self, struct i2c_bus *bus, uint8_t addr)
{
	uint16_t id;
	int ret;

	if (!self || !bus) {
		return -EINVAL;
	}

	self->bus = bus;
	self->addr = addr;
	self->ready = false;

	ret = read_reg16(self, TEMP_SENSOR_REG_DEVICE_ID, &id);
	if (ret == -ENXIO) {
		/* Trong ngữ cảnh init, "không ai ACK" nghĩa là "không có cảm biến" */
		return -ENODEV;
	}
	if (ret < 0) {
		return -EIO;
	}

	if (id != TEMP_SENSOR_DEVICE_ID) {
		/* Có chip trả lời nhưng không phải loại ta cần: dùng tiếp sẽ đọc ra rác */
		return -ENODEV;
	}

	self->ready = true;
	return 0;
}

int temp_sensor_read(struct temp_sensor *self, int32_t *out_mdeg_c)
{
	uint16_t cfg;
	uint16_t raw;

	if (!self || !out_mdeg_c || !self->ready) {
		return -EINVAL;
	}

	/*
	 * Mọi lỗi bus (-ENXIO, -ETIMEDOUT, ...) đều đổi thành -EIO: caller của
	 * temp_sensor_read chỉ cần biết "cảm biến không đọc được", và tập mã lỗi
	 * của hàm này giữ cố định dù sau này đổi sang bus SPI.
	 */
	if (read_reg16(self, TEMP_SENSOR_REG_CONFIG, &cfg) < 0) {
		return -EIO;
	}

	if (!(cfg & TEMP_SENSOR_CFG_DATA_READY)) {
		return -EAGAIN;
	}

	if (read_reg16(self, TEMP_SENSOR_REG_TEMP, &raw) < 0) {
		return -EIO;
	}

	/*
	 * 1 LSB = 7.8125 m°C = 125/16 m°C; raw là số có dấu bù 2. Nhân 125 rồi chia 16
	 * thay vì nhân 78125 chia 10000: 32767 * 78125 tràn int32, 32767 * 125 thì không.
	 */
	*out_mdeg_c = (int32_t)(int16_t)raw * 125 / 16;
	return 0;
}
