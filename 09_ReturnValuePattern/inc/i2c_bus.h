#ifndef I2C_BUS_H
#define I2C_BUS_H

#include <stddef.h>
#include <stdint.h>

#include "must_check.h"

#define I2C_BUS_SIM_NREGS 16

/*
 * Bus I2C mô phỏng trên PC: có đúng một thiết bị giả gắn ở địa chỉ sim_addr,
 * với bảng thanh ghi 16-bit. Các field sim_* chỉ dùng để giả lập phần cứng
 * và tiêm lỗi trong demo/test; driver thật sẽ có thanh ghi peripheral ở đây.
 */
struct i2c_bus {
	const char *name;
	uint8_t sim_addr;
	uint16_t sim_regs[I2C_BUS_SIM_NREGS];
	int sim_fail_err;         /* lỗi sẽ trả cho các lần truyền kế tiếp (0 = không lỗi) */
	unsigned sim_fail_count;  /* còn bao nhiêu lần truyền bị lỗi */
};

MUST_CHECK int i2c_bus_init(struct i2c_bus *self, const char *name);

/*
 * Ghi wlen byte rồi đọc rlen byte (kiểu "write register pointer, read data").
 *
 * Trả về:
 *   0           thành công
 *   -EINVAL     tham số sai
 *   -ENXIO      không thiết bị nào ACK địa chỉ addr
 *   -ETIMEDOUT  bus bị treo (SCL bị giữ thấp quá lâu)
 */
MUST_CHECK int i2c_bus_write_read(struct i2c_bus *self, uint8_t addr, const uint8_t *wbuf,
				  size_t wlen, uint8_t *rbuf, size_t rlen);

/* Móc giả lập (chỉ có trên PC) */
void i2c_bus_sim_attach(struct i2c_bus *self, uint8_t addr);
void i2c_bus_sim_set_reg(struct i2c_bus *self, uint8_t reg, uint16_t value);
void i2c_bus_sim_fail_next(struct i2c_bus *self, int err, unsigned count);

#endif /* I2C_BUS_H */
