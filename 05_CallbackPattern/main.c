#include "button.h"
#include "press_counter.h"
#include "status_led.h"

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

/* Giá trị số của errno khác nhau giữa các nền tảng, nên in tên cho dễ đọc */
static const char *err_name(int err)
{
	switch (err) {
	case 0:
		return "0";
	case -EINVAL:
		return "-EINVAL";
	case -EALREADY:
		return "-EALREADY";
	case -EBUSY:
		return "-EBUSY";
	case -ENOENT:
		return "-ENOENT";
	default:
		return "?";
	}
}

/* Mô phỏng ISR của chân GPIO: trên chip thật, hàm này do vector table gọi */
static void gpio_irq(struct button *btn, bool pressed)
{
	printf("GPIO IRQ: %s %s\n", btn->name, pressed ? "xuống (nhấn)" : "lên (nhả)");
	button_set_level(btn, pressed);
}

int main(void)
{
	/*
	 * Object nằm trong .bss giống firmware thật; main là nơi duy nhất "nối dây".
	 * button.c không include status_led.h hay press_counter.h.
	 */
	static struct button btn_user;
	static struct button btn_mode;
	static struct status_led led;
	static struct press_counter cnt_user;
	static struct press_counter cnt_mode;

	button_init(&btn_user, "BTN_USER");
	button_init(&btn_mode, "BTN_MODE");
	status_led_init(&led, "LED1", &btn_user);
	press_counter_init(&cnt_user, "CNT_USER", &btn_user);
	press_counter_init(&cnt_mode, "CNT_MODE", &btn_mode);

	printf("offsetof(struct status_led, on_button)    = %u\n",
	       (unsigned)offsetof(struct status_led, on_button));
	printf("offsetof(struct press_counter, on_button) = %u\n\n",
	       (unsigned)offsetof(struct press_counter, on_button));

	gpio_irq(&btn_user, true);
	gpio_irq(&btn_user, false);
	gpio_irq(&btn_mode, true);
	gpio_irq(&btn_mode, true); /* cùng mức: không phải sự kiện mới */
	gpio_irq(&btn_user, true);

	printf("\nđăng ký LED1 lần 2: %s\n",
	       err_name(button_add_callback(&btn_user, &led.on_button)));
	printf("button_deinit(BTN_USER) khi còn observer: %s\n\n",
	       err_name(button_deinit(&btn_user)));

	press_counter_deinit(&cnt_user);
	printf("đã gỡ CNT_USER\n");
	gpio_irq(&btn_user, false);
	gpio_irq(&btn_user, true);

	printf("\nLED1 %s, CNT_MODE presses=%lu\n", status_led_is_on(&led) ? "ON" : "OFF",
	       (unsigned long)press_counter_presses(&cnt_mode));

	/* Thứ tự hủy: observer trước, subject sau */
	status_led_deinit(&led);
	press_counter_deinit(&cnt_mode);
	printf("button_deinit(BTN_USER) = %s\n", err_name(button_deinit(&btn_user)));
	printf("button_deinit(BTN_MODE) = %s\n", err_name(button_deinit(&btn_mode)));
	return 0;
}
