#include "led_group.h"
#include "container_of.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

int led_group_init(struct led_group *self, const char *name)
{
	if (!self || !name) {
		return -EINVAL;
	}

	memset(self, 0, sizeof(*self));
	self->name = name;
	slist_init(&self->leds);
	return 0;
}

int led_group_add(struct led_group *self, struct led *led)
{
	if (!self || !led) {
		return -EINVAL;
	}

	/* Một node chỉ nằm được trong một danh sách: thêm hai lần là thành vòng tròn */
	if (slist_contains(&self->leds, &led->node)) {
		return -EALREADY;
	}

	slist_append(&self->leds, &led->node);
	return 0;
}

int led_group_remove(struct led_group *self, struct led *led)
{
	if (!self || !led) {
		return -EINVAL;
	}

	return slist_find_and_remove(&self->leds, &led->node) ? 0 : -ENOENT;
}

int led_group_all_off(struct led_group *self)
{
	struct slist_node *node;
	struct slist_node *next;

	if (!self) {
		return -EINVAL;
	}

	SLIST_FOR_EACH_NODE_SAFE(&self->leds, node, next) {
		/* Từ phần "slist_node" đi ngược ra cả struct led */
		struct led *led = CONTAINER_OF(node, struct led, node);

		led_off(led);
		printf("  [%s] tắt %s\n", self->name, led->name);
	}
	return 0;
}

size_t led_group_count_on(const struct led_group *self)
{
	size_t count = 0;

	if (!self) {
		return 0;
	}

	for (const struct slist_node *node = self->leds.head; node; node = node->next) {
		const struct led *led = CONTAINER_OF(node, struct led, node);

		if (led_is_on(led)) {
			count++;
		}
	}
	return count;
}
