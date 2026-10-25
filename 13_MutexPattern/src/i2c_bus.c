#include "i2c_bus.h"
#include "i2c_sim.h"

#include <errno.h>

/*
 * Chỉ dùng cho thí nghiệm trong bài giảng: build với EXTRA=-DI2C_BUS_DEMO_NO_LOCK
 * để transfer chạy KHÔNG khóa và tận mắt thấy race. Code thật không bao giờ có cờ này.
 */
#ifdef I2C_BUS_DEMO_NO_LOCK
#define XFER_LOCK(self) 0
#define XFER_UNLOCK(self) ((void)0)
#else
#define XFER_LOCK(self) mutex_lock(&(self)->lock, MUTEX_FOREVER)
#define XFER_UNLOCK(self) mutex_unlock(&(self)->lock)
#endif

int i2c_bus_init(struct i2c_bus *self, const char *name)
{
	if (!self || !name) {
		return -EINVAL;
	}

	self->name = name;
	atomic_init(&self->hw_target, 0);
	self->transfers = 0;
	return mutex_init(&self->lock);
}

int i2c_bus_deinit(struct i2c_bus *self)
{
	if (!self) {
		return -EINVAL;
	}

	return mutex_deinit(&self->lock);
}

/* START + địa chỉ: ghi thanh ghi đích của khối I2C (dùng chung cho cả bus) */
static void hw_start(struct i2c_bus *self, uint8_t addr)
{
	atomic_store(&self->hw_target, addr);
	i2c_sim_byte_delay();
}

int i2c_bus_write_read(struct i2c_bus *self, uint8_t addr, uint8_t reg, uint8_t *buf, size_t len)
{
	if (!self || !buf || len == 0) {
		return -EINVAL;
	}

	int ret = XFER_LOCK(self);
	if (ret) {
		return ret;
	}

	/*
	 * Từ đây tới XFER_UNLOCK là critical section: cả chuỗi
	 * START-addr-reg-RESTART-addr-đọc phải liền mạch, nếu không
	 * thread khác đổi hw_target giữa chừng và ta đọc nhầm thiết bị.
	 */
	hw_start(self, addr);
	ret = i2c_sim_set_pointer(atomic_load(&self->hw_target), reg);
	i2c_sim_byte_delay();

	hw_start(self, addr); /* repeated START, chuyển sang đọc */
	for (size_t i = 0; i < len && ret == 0; i++) {
		ret = i2c_sim_read_next(atomic_load(&self->hw_target), &buf[i]);
		i2c_sim_byte_delay();
	}
	self->transfers++;

	XFER_UNLOCK(self);
	return ret;
}

int i2c_bus_write(struct i2c_bus *self, uint8_t addr, uint8_t reg, const uint8_t *data, size_t len)
{
	if (!self || !data || len == 0) {
		return -EINVAL;
	}

	int ret = XFER_LOCK(self);
	if (ret) {
		return ret;
	}

	hw_start(self, addr);
	ret = i2c_sim_set_pointer(atomic_load(&self->hw_target), reg);
	i2c_sim_byte_delay();
	for (size_t i = 0; i < len && ret == 0; i++) {
		ret = i2c_sim_write_next(atomic_load(&self->hw_target), data[i]);
		i2c_sim_byte_delay();
	}
	self->transfers++;

	XFER_UNLOCK(self);
	return ret;
}

int i2c_bus_lock(struct i2c_bus *self, int32_t timeout_ms)
{
	if (!self) {
		return -EINVAL;
	}

	return mutex_lock(&self->lock, timeout_ms);
}

int i2c_bus_unlock(struct i2c_bus *self)
{
	if (!self) {
		return -EINVAL;
	}

	return mutex_unlock(&self->lock);
}
