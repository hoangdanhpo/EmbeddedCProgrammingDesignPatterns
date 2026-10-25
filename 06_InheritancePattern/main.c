#include "battery_monitor.h"
#include "led.h"
#include "led_group.h"
#include "power_trait.h"
#include "sensor_trait.h"
#include "tmp36_sensor.h"

#include <errno.h>
#include <stddef.h>
#include <stdio.h>

static void print_led(const struct led *led)
{
	printf("  %-10s %s%u active_low=%d -> logic %-3s, mức điện trên chân = %d\n", led->name,
	       led->pin.port, (unsigned)led->pin.pin, led->pin.active_low,
	       led_is_on(led) ? "ON" : "OFF", gpio_pin_level(&led->pin));
}

static void demo_single_inheritance(struct led *status, struct led *error)
{
	printf("1. Kế thừa + delegator: struct led chứa struct gpio_pin\n");
	led_on(status);
	led_on(error);
	print_led(status);
	print_led(error);
	printf("  offsetof(struct led, pin) = %u, offsetof(struct led, node) = %u\n\n",
	       (unsigned)offsetof(struct led, pin), (unsigned)offsetof(struct led, node));
}

static void demo_list_inheritance(struct led_group *group, struct led *status, struct led *wifi)
{
	printf("2. Kế thừa slist_node: thuật toán danh sách viết một lần\n");
	led_toggle(wifi);
	printf("  số LED đang sáng: %u\n", (unsigned)led_group_count_on(group));
	led_group_all_off(group);
	printf("  số LED đang sáng: %u\n", (unsigned)led_group_count_on(group));
	printf("  thêm LED_STATUS vào nhóm lần 2: %s\n\n",
	       led_group_add(group, status) == -EALREADY ? "-EALREADY" : "?");
}

/* Code dùng chung: chỉ biết "trait", không biết TMP36 hay battery là gì */
struct sensor_entry {
	const char *name;
	const char *unit;
	struct sensor_trait *sensor;
};

static void read_all(const struct sensor_entry *entries, size_t count)
{
	for (size_t i = 0; i < count; i++) {
		int32_t value = 0;
		int err = sensor_trait_read(entries[i].sensor, &value);

		if (err == -EAGAIN) {
			printf("  %-8s -EAGAIN (đang tắt nguồn)\n", entries[i].name);
		} else if (err) {
			printf("  %-8s lỗi %d\n", entries[i].name, err);
		} else {
			printf("  %-8s %ld %s\n", entries[i].name, (long)value, entries[i].unit);
		}
	}
}

static void demo_traits(struct tmp36_sensor *temp, struct battery_monitor *battery)
{
	const struct sensor_entry sensors[] = {
		{ "TEMP", "x0.1 °C", &temp->sensor },
		{ "BATTERY", "mV", &battery->sensor },
	};
	/* Chỉ TMP36 có power_trait: battery_monitor không "là" thiết bị tắt được */
	struct power_trait *power_devices[] = { &temp->power };
	const size_t n_sensors = sizeof(sensors) / sizeof(sensors[0]);
	const size_t n_power = sizeof(power_devices) / sizeof(power_devices[0]);

	printf("3. Trait: đối xử chung với mọi kiểu có cùng trait\n");
	tmp36_sensor_sim_set_mv(temp, 753);      /* 753 mV -> 25.3 °C */
	battery_monitor_sim_set_mv(battery, 1850); /* pin 3700 mV */
	read_all(sensors, n_sensors);

	printf("  suspend mọi thiết bị có power_trait\n");
	for (size_t i = 0; i < n_power; i++) {
		power_trait_suspend(power_devices[i]);
	}
	read_all(sensors, n_sensors);

	printf("  resume\n");
	for (size_t i = 0; i < n_power; i++) {
		power_trait_resume(power_devices[i]);
	}
	read_all(sensors, n_sensors);

	printf("  offsetof(struct tmp36_sensor, sensor/power/power_pin) = %u/%u/%u\n",
	       (unsigned)offsetof(struct tmp36_sensor, sensor),
	       (unsigned)offsetof(struct tmp36_sensor, power),
	       (unsigned)offsetof(struct tmp36_sensor, power_pin));
}

int main(void)
{
	/* Cây object tĩnh, nối dây ở một chỗ (Object Pattern, bài 1) */
	static struct led status;
	static struct led error;
	static struct led wifi;
	static struct led_group all;
	static struct tmp36_sensor board_temp;
	static struct battery_monitor battery;

	led_init(&status, "LED_STATUS", "PA", 5, false);
	led_init(&error, "LED_ERROR", "PB", 0, true); /* LED nối lên VCC: mức 0 mới sáng */
	led_init(&wifi, "LED_WIFI", "PB", 7, false);
	led_group_init(&all, "ALL");
	led_group_add(&all, &status);
	led_group_add(&all, &error);
	led_group_add(&all, &wifi);
	tmp36_sensor_init(&board_temp, "PC", 3);
	battery_monitor_init(&battery);

	demo_single_inheritance(&status, &error);
	demo_list_inheritance(&all, &status, &wifi);
	demo_traits(&board_temp, &battery);
	return 0;
}
