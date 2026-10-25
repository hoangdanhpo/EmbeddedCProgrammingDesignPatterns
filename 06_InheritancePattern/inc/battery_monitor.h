#ifndef BATTERY_MONITOR_H
#define BATTERY_MONITOR_H

#include "sensor_trait.h"

#include <stdint.h>

/*
 * Đo điện áp pin qua cầu phân áp 1:2. Chỉ có trait sensor: không tắt nguồn
 * được, nên không có power_trait.
 */
struct battery_monitor {
	struct sensor_trait sensor;
	uint16_t adc_mv; /* Mô phỏng ADC */
};

int battery_monitor_init(struct battery_monitor *self);
void battery_monitor_sim_set_mv(struct battery_monitor *self, uint16_t mv);

#endif /* BATTERY_MONITOR_H */
