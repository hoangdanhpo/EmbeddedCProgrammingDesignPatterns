#include "i2c_bus.h"

#include <errno.h>
#include <string.h>

int i2c_bus_init(struct i2c_bus *self, const char *name)
{
	if (!self || !name) {
		return -EINVAL;
	}

	memset(self, 0, sizeof(*self));
	self->name = name;
	return 0;
}

int i2c_bus_write_read(struct i2c_bus *self, uint8_t addr, const uint8_t *wbuf, size_t wlen,
		       uint8_t *rbuf, size_t rlen)
{
	if (!self || (wlen && !wbuf) || (rlen && !rbuf) || addr > 0x7F) {
		return -EINVAL;
	}

	if (self->sim_fail_count > 0) {
		self->sim_fail_count--;
		return self->sim_fail_err;
	}

	/* Không ai kéo SDA xuống ở bit ACK: chuẩn errno gọi là "No such device or address" */
	if (self->sim_addr == 0 || addr != self->sim_addr) {
		return -ENXIO;
	}

	/* Thiết bị giả: byte ghi đầu tiên là con trỏ thanh ghi, dữ liệu 16-bit big-endian */
	if (wlen < 1 || wbuf[0] >= I2C_BUS_SIM_NREGS) {
		return -EINVAL;
	}

	uint16_t value = self->sim_regs[wbuf[0]];

	for (size_t i = 0; i < rlen; i++) {
		rbuf[i] = (i % 2 == 0) ? (uint8_t)(value >> 8) : (uint8_t)(value & 0xFF);
	}
	return 0;
}

void i2c_bus_sim_attach(struct i2c_bus *self, uint8_t addr)
{
	self->sim_addr = addr;
}

void i2c_bus_sim_set_reg(struct i2c_bus *self, uint8_t reg, uint16_t value)
{
	if (reg < I2C_BUS_SIM_NREGS) {
		self->sim_regs[reg] = value;
	}
}

void i2c_bus_sim_fail_next(struct i2c_bus *self, int err, unsigned count)
{
	self->sim_fail_err = err;
	self->sim_fail_count = count;
}
