#include "gpio_pin.h"

#include <errno.h>
#include <string.h>

int gpio_pin_init(struct gpio_pin *self, const char *port, uint8_t pin, bool active_low)
{
	if (!self || !port || pin > 15) {
		return -EINVAL;
	}

	memset(self, 0, sizeof(*self));
	self->port = port;
	self->pin = pin;
	self->active_low = active_low;
	/* Khởi động ở trạng thái "không active": active-low thì phải kéo lên mức 1 */
	self->level = active_low;
	return 0;
}

int gpio_pin_set_active(struct gpio_pin *self, bool active)
{
	if (!self) {
		return -EINVAL;
	}

	/* Chỉ một chỗ duy nhất biết quy tắc đảo mức: lớp dẫn xuất không phải lo */
	self->level = active != self->active_low;
	return 0;
}

bool gpio_pin_is_active(const struct gpio_pin *self)
{
	return self ? self->level != self->active_low : false;
}

bool gpio_pin_level(const struct gpio_pin *self)
{
	return self ? self->level : false;
}
