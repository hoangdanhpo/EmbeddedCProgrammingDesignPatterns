#include "battery_monitor.h"
#include "container_of.h"

#include <errno.h>
#include <string.h>

static int battery_monitor_read(struct sensor_trait *trait, int32_t *value)
{
	struct battery_monitor *self = CONTAINER_OF(trait, struct battery_monitor, sensor);

	/* Cầu phân áp 1:2: điện áp pin gấp đôi điện áp ADC thấy (đơn vị mV) */
	*value = (int32_t)self->adc_mv * 2;
	return 0;
}

int battery_monitor_init(struct battery_monitor *self)
{
	if (!self) {
		return -EINVAL;
	}

	memset(self, 0, sizeof(*self));
	self->sensor.read = battery_monitor_read;
	return 0;
}

void battery_monitor_sim_set_mv(struct battery_monitor *self, uint16_t mv)
{
	if (self) {
		self->adc_mv = mv;
	}
}
