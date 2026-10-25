#ifndef I2C_SIM_H
#define I2C_SIM_H

/*
 * Mô phỏng phần cứng phía sau bus I2C: một cảm biến TMP102 và một EEPROM 24C02.
 * Header private (nằm trong src/): chỉ i2c_bus.c được thấy "phần cứng".
 */

#include <stdint.h>

#define I2C_SIM_TMP102_ADDR 0x48
#define I2C_SIM_EEPROM_ADDR 0x50

/* Byte đầu tiên sau địa chỉ khi ghi: đặt con trỏ thanh ghi bên trong thiết bị */
int i2c_sim_set_pointer(uint8_t addr, uint8_t reg);

/* Đọc/ghi một byte tại con trỏ hiện tại của thiết bị, rồi con trỏ tự tăng */
int i2c_sim_read_next(uint8_t addr, uint8_t *out);
int i2c_sim_write_next(uint8_t addr, uint8_t val);

/* Thời gian truyền một byte trên dây; CPU bận chờ cờ như một driver polling */
void i2c_sim_byte_delay(void);

#endif /* I2C_SIM_H */
