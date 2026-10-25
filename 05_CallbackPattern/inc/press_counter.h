#ifndef PRESS_COUNTER_H
#define PRESS_COUNTER_H

#include "button.h"

#include <stdint.h>

/*
 * Observer 2: đếm số lần nhấn và nhả. Cùng một hàm handler phục vụ mọi
 * instance; mỗi instance có state riêng nhờ CONTAINER_OF.
 */
struct press_counter {
	const char *name;
	struct button *btn;
	uint32_t presses;
	uint32_t releases;
	struct button_callback on_button;
};

int press_counter_init(struct press_counter *self, const char *name, struct button *btn);
int press_counter_deinit(struct press_counter *self);
uint32_t press_counter_presses(const struct press_counter *self);

#endif /* PRESS_COUNTER_H */
