#ifndef LED_GROUP_H
#define LED_GROUP_H

#include "led.h"
#include "slist.h"

#include <stddef.h>

/*
 * Thuật toán viết trên "base" slist, dùng cho mọi struct led nhờ LED kế thừa
 * slist_node. Danh sách CHỈ được chứa struct led: khi duyệt, CONTAINER_OF tin
 * rằng mọi node đều nằm trong một struct led.
 */
struct led_group {
	const char *name;
	struct slist leds;
};

int led_group_init(struct led_group *self, const char *name);
int led_group_add(struct led_group *self, struct led *led);
int led_group_remove(struct led_group *self, struct led *led);
int led_group_all_off(struct led_group *self);
size_t led_group_count_on(const struct led_group *self);

#endif /* LED_GROUP_H */
