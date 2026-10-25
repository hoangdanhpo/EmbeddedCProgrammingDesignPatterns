#ifndef GPIO_PIN_H
#define GPIO_PIN_H

#include <stdbool.h>
#include <stdint.h>

/*
 * Base class: một chân GPIO output. Biết chuyện "active-low" (mức 0 mới là
 * bật) để mọi lớp dẫn xuất (LED, relay, chân cấp nguồn cảm biến...) chỉ cần
 * nói "bật/tắt" theo nghĩa logic.
 */
struct gpio_pin {
	const char *port;
	uint8_t pin;
	bool active_low;
	bool level; /* Mức điện thật trên chân (mô phỏng thanh ghi ODR) */
};

int gpio_pin_init(struct gpio_pin *self, const char *port, uint8_t pin, bool active_low);
int gpio_pin_set_active(struct gpio_pin *self, bool active);
bool gpio_pin_is_active(const struct gpio_pin *self);
bool gpio_pin_level(const struct gpio_pin *self);

#endif /* GPIO_PIN_H */
