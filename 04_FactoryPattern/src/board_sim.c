#include "board_sim.h"

#include <errno.h>
#include <stddef.h>
#include <stdio.h>

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

/* Thiết bị I2C "có mặt" trên board giả lập; 0x4A, 0x4B không được hàn */
struct sim_i2c_dev {
	uint8_t addr;
	uint16_t temp_reg;   /* thanh ghi 0x00 của TMP102: 12 bit căn trái */
	uint16_t config_reg; /* thanh ghi 0x01 */
};

static struct sim_i2c_dev sim_i2c_devs[] = {
	{ .addr = 0x48, .temp_reg = 0x1900, .config_reg = 0x60A0 }, /* 400 LSB = +25.000 °C */
	{ .addr = 0x49, .temp_reg = 0xE700, .config_reg = 0x60A0 }, /* -400 LSB = -25.000 °C */
};

/* Giá trị thô 12 bit trên từng kênh ADC */
static const uint16_t sim_adc_raw[16] = {
	[3] = 372, /* LM35 ở kênh 3: ~300 mV với Vref 3.3 V, tức ~30 °C */
	[5] = 124, /* ~100 mV, tức ~10 °C */
};

static struct sim_i2c_dev *sim_find(uint8_t addr)
{
	for (size_t i = 0; i < ARRAY_SIZE(sim_i2c_devs); i++) {
		if (sim_i2c_devs[i].addr == addr) {
			return &sim_i2c_devs[i];
		}
	}
	return NULL;
}

int board_i2c_read_u16(uint8_t addr, uint8_t reg, uint16_t *out)
{
	struct sim_i2c_dev *dev = sim_find(addr);

	if (!dev) {
		return -EIO; /* không có ACK: chip không có mặt ở địa chỉ này */
	}
	*out = (reg == 0x00) ? dev->temp_reg : dev->config_reg;
	return 0;
}

int board_i2c_write_u16(uint8_t addr, uint8_t reg, uint16_t value)
{
	struct sim_i2c_dev *dev = sim_find(addr);

	if (!dev) {
		return -EIO;
	}
	if (reg == 0x01) {
		dev->config_reg = value;
	}
	printf("  [sim i2c 0x%02X] write reg 0x%02X = 0x%04X\n", (unsigned)addr, (unsigned)reg,
	       (unsigned)value);
	return 0;
}

int board_adc_read(uint8_t channel, uint16_t *out_raw)
{
	if (channel >= ARRAY_SIZE(sim_adc_raw)) {
		return -EINVAL;
	}
	*out_raw = sim_adc_raw[channel];
	return 0;
}
