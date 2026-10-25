#ifndef STATUS_LED_H
#define STATUS_LED_H

#include "button.h"

#include <stdbool.h>

/*
 * Observer 1: đảo trạng thái LED mỗi lần nút được nhấn.
 * on_button được NHÚNG (không phải con trỏ) để handler dùng CONTAINER_OF
 * đi ngược từ &self->on_button về self.
 */
struct status_led {
	const char *name;
	struct button *btn;
	struct button_callback on_button;
	bool on;
};

int status_led_init(struct status_led *self, const char *name, struct button *btn);
int status_led_deinit(struct status_led *self);
bool status_led_is_on(const struct status_led *self);

#endif /* STATUS_LED_H */
