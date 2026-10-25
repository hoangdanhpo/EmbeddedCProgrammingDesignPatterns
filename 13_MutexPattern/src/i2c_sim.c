#include "i2c_sim.h"

#include <errno.h>
#include <stdatomic.h>
#include <time.h>

/*
 * Thanh ghi TMP102, mỗi thanh ghi 16 bit (2 byte, MSB trước):
 * 0 = nhiệt độ 0x1900 (25.0 độ C), 1 = config 0x60A0 (mặc định), 2 = T_LOW, 3 = T_HIGH.
 * Phần cứng là state toàn cục thật sự, nên mô phỏng bằng biến static là hợp lý.
 */
static const uint8_t tmp102_regs[8] = { 0x19, 0x00, 0x60, 0xA0, 0x4B, 0x00, 0x50, 0x00 };
static _Atomic uint8_t tmp102_ptr;

static _Atomic uint8_t eeprom_mem[256];
static _Atomic uint8_t eeprom_ptr;

int i2c_sim_set_pointer(uint8_t addr, uint8_t reg)
{
	switch (addr) {
	case I2C_SIM_TMP102_ADDR:
		atomic_store(&tmp102_ptr, (uint8_t)((reg & 0x03u) * 2u));
		return 0;
	case I2C_SIM_EEPROM_ADDR:
		atomic_store(&eeprom_ptr, reg);
		return 0;
	default:
		return -ENXIO; /* Không thiết bị nào ACK địa chỉ này */
	}
}

int i2c_sim_read_next(uint8_t addr, uint8_t *out)
{
	switch (addr) {
	case I2C_SIM_TMP102_ADDR:
		*out = tmp102_regs[atomic_fetch_add(&tmp102_ptr, 1) & 0x07u];
		return 0;
	case I2C_SIM_EEPROM_ADDR:
		*out = atomic_load(&eeprom_mem[atomic_fetch_add(&eeprom_ptr, 1)]);
		return 0;
	default:
		return -ENXIO;
	}
}

int i2c_sim_write_next(uint8_t addr, uint8_t val)
{
	switch (addr) {
	case I2C_SIM_TMP102_ADDR:
		return 0; /* Mô phỏng đơn giản: bỏ qua ghi vào cảm biến */
	case I2C_SIM_EEPROM_ADDR:
		atomic_store(&eeprom_mem[atomic_fetch_add(&eeprom_ptr, 1)], val);
		return 0;
	default:
		return -ENXIO;
	}
}

void i2c_sim_byte_delay(void)
{
	/* ~20 us. Không dùng sleep: trên Windows sleep nhỏ nhất ~15 ms */
	struct timespec start;
	struct timespec now;

	timespec_get(&start, TIME_UTC);
	do {
		timespec_get(&now, TIME_UTC);
	} while ((now.tv_sec - start.tv_sec) * 1000000000L + (now.tv_nsec - start.tv_nsec) < 20000L);
}
