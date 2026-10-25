#include "led.h"

#include <errno.h>
#include <string.h>

int led_init(struct led *self, const char *name, const char *port, uint8_t pin, bool active_low)
{
	if (!self || !name) {
		return -EINVAL;
	}

	memset(self, 0, sizeof(*self));
	self->name = name;
	/* Khởi tạo lớp dẫn xuất = khởi tạo từng base, rồi tới phần riêng */
	return gpio_pin_init(&self->pin, port, pin, active_low);
}

int led_on(struct led *self)
{
	if (!self) {
		return -EINVAL;
	}

	/* Không ghi self->pin.level trực tiếp: quy tắc active-low thuộc về base */
	return gpio_pin_set_active(&self->pin, true);
}

int led_off(struct led *self)
{
	if (!self) {
		return -EINVAL;
	}

	return gpio_pin_set_active(&self->pin, false);
}

int led_toggle(struct led *self)
{
	if (!self) {
		return -EINVAL;
	}

	return gpio_pin_set_active(&self->pin, !gpio_pin_is_active(&self->pin));
}

bool led_is_on(const struct led *self)
{
	return self ? gpio_pin_is_active(&self->pin) : false;
}
