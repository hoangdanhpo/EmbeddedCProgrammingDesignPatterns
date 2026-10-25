#include "slow_sensor.h"
#include "systime.h"

#include <errno.h>

int slow_sensor_init(struct slow_sensor *self, uint32_t conversion_ms)
{
	if (!self) {
		return -EINVAL;
	}

	self->conversion_ms = conversion_ms;
	self->sim_mdeg = 21500;
	self->reads = 0;
	return 0;
}

int slow_sensor_read(struct slow_sensor *self, int32_t *out_mdeg_c)
{
	if (!self || !out_mdeg_c) {
		return -EINVAL;
	}

	/* Mô phỏng "ra lệnh đo rồi chờ": CPU (thread này) không làm được gì khác */
	systime_sleep_ms(self->conversion_ms);

	*out_mdeg_c = self->sim_mdeg;
	self->sim_mdeg += 125;
	self->reads++;
	return 0;
}
