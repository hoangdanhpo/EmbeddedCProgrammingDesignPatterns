#include "heartbeat.h"
#include "systime.h"

#include <errno.h>
#include <string.h>

int heartbeat_init(struct heartbeat *self, uint32_t period_ms, uint32_t now_ms)
{
	if (!self || period_ms == 0) {
		return -EINVAL;
	}

	memset(self, 0, sizeof(*self));
	self->period_ms = period_ms;
	self->start_ms = now_ms;
	self->last_toggle_ms = now_ms;
	self->next_due_ms = now_ms + period_ms;
	memset(self->timeline, '.', HEARTBEAT_MAX_SLOTS);
	self->timeline[HEARTBEAT_MAX_SLOTS] = '\0';
	return 0;
}

int heartbeat_poll(struct heartbeat *self, uint32_t now_ms)
{
	if (!self) {
		return -EINVAL;
	}

	if (!systime_reached(now_ms, self->next_due_ms)) {
		return 0;
	}

	uint32_t interval = now_ms - self->last_toggle_ms;
	uint32_t slot = (now_ms - self->start_ms) / self->period_ms;

	self->led_on = !self->led_on; /* trên MCU: gpio_pin_toggle() */
	self->toggles++;
	if (interval > self->max_interval_ms) {
		self->max_interval_ms = interval;
	}
	if (interval > self->period_ms + self->period_ms / 2) {
		self->late++;
	}
	if (slot < HEARTBEAT_MAX_SLOTS) {
		self->timeline[slot] = '#';
	}

	self->last_toggle_ms = now_ms;
	/* Hẹn lần sau tính từ bây giờ, không cộng dồn: trễ rồi thì không "đảo bù" liên tục */
	self->next_due_ms = now_ms + self->period_ms;
	return 1;
}
