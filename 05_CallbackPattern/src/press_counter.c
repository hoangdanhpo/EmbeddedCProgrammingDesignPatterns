#include "press_counter.h"
#include "container_of.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

static void press_counter_on_button(struct button_callback *cb, enum button_event event)
{
	struct press_counter *self = CONTAINER_OF(cb, struct press_counter, on_button);

	if (event == BUTTON_EVENT_PRESSED) {
		self->presses++;
	} else {
		self->releases++;
	}
	printf("  [%s] %s: presses=%lu releases=%lu\n", self->name,
	       event == BUTTON_EVENT_PRESSED ? "PRESSED " : "RELEASED",
	       (unsigned long)self->presses, (unsigned long)self->releases);
}

int press_counter_init(struct press_counter *self, const char *name, struct button *btn)
{
	if (!self || !name || !btn) {
		return -EINVAL;
	}

	memset(self, 0, sizeof(*self));
	self->name = name;
	self->btn = btn;
	self->on_button.handler = press_counter_on_button;
	return button_add_callback(btn, &self->on_button);
}

int press_counter_deinit(struct press_counter *self)
{
	if (!self) {
		return -EINVAL;
	}

	int err = button_remove_callback(self->btn, &self->on_button);

	memset(self, 0, sizeof(*self));
	return err;
}

uint32_t press_counter_presses(const struct press_counter *self)
{
	return self ? self->presses : 0;
}
