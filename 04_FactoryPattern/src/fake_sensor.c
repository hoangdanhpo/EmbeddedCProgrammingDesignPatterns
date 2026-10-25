#include "fake_sensor.h"

#include <errno.h>
#include <string.h>

static int fake_sensor_read(sensor_t sensor, int32_t *out_milli_c)
{
	struct fake_sensor *self = CONTAINER_OF(sensor, struct fake_sensor, base.api);

	self->reads++;
	*out_milli_c = self->value_milli_c;
	return 0;
}

static const struct sensor_api fake_sensor_api = {
	.read = fake_sensor_read,
	.deinit = NULL,
};

int fake_sensor_init(struct fake_sensor *self, const char *name, int32_t value_milli_c)
{
	if (!self || !name) {
		return -EINVAL;
	}

	memset(self, 0, sizeof(*self));
	self->base.api = &fake_sensor_api;
	self->base.name = name;
	self->value_milli_c = value_milli_c;
	return 0;
}
