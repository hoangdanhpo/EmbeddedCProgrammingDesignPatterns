#include "tmp36_sensor.h"
#include "container_of.h"

#include <errno.h>
#include <string.h>

/*
 * Các hàm trait đều static: chỉ được gọi qua con trỏ hàm trong trait, nên
 * không lộ tên ra ngoài file này.
 */
static int tmp36_sensor_read(struct sensor_trait *trait, int32_t *value)
{
	struct tmp36_sensor *self = CONTAINER_OF(trait, struct tmp36_sensor, sensor);

	if (!gpio_pin_is_active(&self->power_pin)) {
		return -EAGAIN; /* cảm biến đang tắt nguồn: không có số đo */
	}

	/* Đơn vị 0.1 °C: (mV - 500) / 10 °C = (mV - 500) * 0.1 °C */
	*value = (int32_t)self->adc_mv - 500;
	return 0;
}

static int tmp36_sensor_suspend(struct power_trait *trait)
{
	struct tmp36_sensor *self = CONTAINER_OF(trait, struct tmp36_sensor, power);

	return gpio_pin_set_active(&self->power_pin, false);
}

static int tmp36_sensor_resume(struct power_trait *trait)
{
	struct tmp36_sensor *self = CONTAINER_OF(trait, struct tmp36_sensor, power);

	return gpio_pin_set_active(&self->power_pin, true);
}

int tmp36_sensor_init(struct tmp36_sensor *self, const char *port, uint8_t pin)
{
	if (!self) {
		return -EINVAL;
	}

	memset(self, 0, sizeof(*self));
	/* Mỗi instance tự giữ bản sao con trỏ hàm của mình (đặc điểm của trait) */
	self->sensor = (struct sensor_trait){ .read = tmp36_sensor_read };
	self->power = (struct power_trait){
		.suspend = tmp36_sensor_suspend,
		.resume = tmp36_sensor_resume,
	};

	int err = gpio_pin_init(&self->power_pin, port, pin, false);

	if (err) {
		return err;
	}
	return gpio_pin_set_active(&self->power_pin, true);
}

void tmp36_sensor_sim_set_mv(struct tmp36_sensor *self, uint16_t mv)
{
	if (self) {
		self->adc_mv = mv;
	}
}
