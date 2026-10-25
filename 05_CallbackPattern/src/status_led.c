#include "status_led.h"
#include "container_of.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

/*
 * static: chỉ button gọi hàm này, qua con trỏ hàm. Không ai khác cần biết
 * tên của nó, nên không đưa vào header.
 */
static void status_led_on_button(struct button_callback *cb, enum button_event event)
{
	struct status_led *self = CONTAINER_OF(cb, struct status_led, on_button);

	if (event != BUTTON_EVENT_PRESSED) {
		return;
	}

	self->on = !self->on;
	printf("  [%s] nút %s nhấn -> LED %s\n", self->name, self->btn->name,
	       self->on ? "ON" : "OFF");
}

int status_led_init(struct status_led *self, const char *name, struct button *btn)
{
	if (!self || !name || !btn) {
		return -EINVAL;
	}

	memset(self, 0, sizeof(*self));
	self->name = name;
	self->btn = btn;
	self->on_button.handler = status_led_on_button;
	return button_add_callback(btn, &self->on_button);
}

int status_led_deinit(struct status_led *self)
{
	if (!self) {
		return -EINVAL;
	}

	/* Phải gỡ callback TRƯỚC khi object biến mất, nếu không button giữ con trỏ treo */
	int err = button_remove_callback(self->btn, &self->on_button);

	memset(self, 0, sizeof(*self));
	return err;
}

bool status_led_is_on(const struct status_led *self)
{
	return self ? self->on : false;
}
