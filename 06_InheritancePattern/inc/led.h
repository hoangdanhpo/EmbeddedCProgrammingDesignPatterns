#ifndef LED_H
#define LED_H

#include "gpio_pin.h"
#include "slist.h"

#include <stdbool.h>
#include <stdint.h>

/*
 * Lớp dẫn xuất, kế thừa HAI base bằng cách chứa chúng:
 * - struct gpio_pin: dữ liệu và hành vi của chân điều khiển LED
 * - struct slist_node: để xếp LED vào danh sách (struct led_group)
 * Tên member nói rõ phần base nào: self->pin là phần gpio_pin, self->node là
 * phần slist_node. Không bao giờ là con trỏ: LED CHỨA base, không trỏ tới base.
 */
struct led {
	struct gpio_pin pin;
	struct slist_node node;
	const char *name;
};

int led_init(struct led *self, const char *name, const char *port, uint8_t pin, bool active_low);

/* Method wrapper (delegator): LED tự có API riêng, bên trong chuyển cho base */
int led_on(struct led *self);
int led_off(struct led *self);
int led_toggle(struct led *self);
bool led_is_on(const struct led *self);

#endif /* LED_H */
