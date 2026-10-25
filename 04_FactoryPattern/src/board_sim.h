#ifndef BOARD_SIM_H
#define BOARD_SIM_H

#include <stdint.h>

/*
 * Mô phỏng bus I2C và ADC của board để demo chạy được trên PC.
 * Trên board thật, đây là driver I2C/ADC (bài 1-2), không phải bảng tra.
 */
int board_i2c_read_u16(uint8_t addr, uint8_t reg, uint16_t *out);
int board_i2c_write_u16(uint8_t addr, uint8_t reg, uint16_t value);
int board_adc_read(uint8_t channel, uint16_t *out_raw);

#endif /* BOARD_SIM_H */
