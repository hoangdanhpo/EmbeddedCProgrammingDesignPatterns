#ifndef I2C_BUS_H
#define I2C_BUS_H

#include "mutex.h"

#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>

/*
 * Một bus I2C dùng chung bởi nhiều driver (cảm biến, EEPROM...) chạy ở nhiều thread.
 * Mutex nằm NGAY TRONG object bus: khóa dữ liệu (bus này), không khóa code
 * (hàm transfer), nên hai bus khác nhau không chặn nhau.
 */
struct i2c_bus {
	const char *name;
	struct mutex lock;
	/*
	 * Mô phỏng thanh ghi địa chỉ đích của khối I2C (trên STM32 là I2C_CR2.SADD).
	 * Cả bus chỉ có MỘT thanh ghi này, nên hai transfer chen nhau sẽ ghi đè lẫn nhau.
	 * _Atomic chỉ để thí nghiệm không có undefined behavior; nó KHÔNG làm transfer an toàn.
	 */
	_Atomic uint8_t hw_target;
	uint32_t transfers; /* Thống kê, chỉ sửa khi đang giữ lock */
};

int i2c_bus_init(struct i2c_bus *self, const char *name);
int i2c_bus_deinit(struct i2c_bus *self);

/* Ghi địa chỉ thanh ghi rồi đọc len byte (repeated start), như đọc cảm biến/EEPROM. */
int i2c_bus_write_read(struct i2c_bus *self, uint8_t addr, uint8_t reg, uint8_t *buf, size_t len);

/* Ghi len byte bắt đầu từ thanh ghi reg. */
int i2c_bus_write(struct i2c_bus *self, uint8_t addr, uint8_t reg, const uint8_t *data, size_t len);

/*
 * Giữ bus qua nhiều transfer liên tiếp (một "giao dịch" của driver).
 * Các transfer bên trong tự khóa lại lần nữa: mutex cho phép khóa lồng nên không deadlock.
 */
int i2c_bus_lock(struct i2c_bus *self, int32_t timeout_ms);
int i2c_bus_unlock(struct i2c_bus *self);

#endif /* I2C_BUS_H */
