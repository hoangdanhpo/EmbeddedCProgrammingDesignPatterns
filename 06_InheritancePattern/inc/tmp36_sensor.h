#ifndef TMP36_SENSOR_H
#define TMP36_SENSOR_H

#include "gpio_pin.h"
#include "power_trait.h"
#include "sensor_trait.h"

#include <stdbool.h>
#include <stdint.h>

/*
 * Cảm biến nhiệt analog TMP36 (Vout = 500 mV + 10 mV/°C), cấp nguồn qua một
 * chân GPIO. Đa kế thừa: hai trait (sensor, power) và một base class (gpio_pin).
 */
struct tmp36_sensor {
	struct sensor_trait sensor;
	struct power_trait power;
	struct gpio_pin power_pin;
	uint16_t adc_mv; /* Mô phỏng kết quả ADC; chip thật sẽ đọc thanh ghi ADC */
};

int tmp36_sensor_init(struct tmp36_sensor *self, const char *port, uint8_t pin);

/* Chỉ dùng trong demo/test để giả lập điện áp đo được */
void tmp36_sensor_sim_set_mv(struct tmp36_sensor *self, uint16_t mv);

#endif /* TMP36_SENSOR_H */
