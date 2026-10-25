#include "app.h"
#include "errstr.h"
#include "i2c_bus.h"
#include "temp_sensor.h"
#include "uart.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

#define SENSOR_ADDR 0x48

/* In mã trả về kèm tên: con số thay đổi theo nền tảng, tên thì không */
static void show(const char *what, int ret)
{
	printf("%-44s -> %4d (%s)\n", what, ret, errstr(ret));
}

/* Đưa cảm biến giả về trạng thái tốt: đúng ID, có dữ liệu, 25.000 °C */
static void sim_sensor_ok(struct i2c_bus *bus)
{
	i2c_bus_sim_attach(bus, SENSOR_ADDR);
	i2c_bus_sim_set_reg(bus, TEMP_SENSOR_REG_DEVICE_ID, TEMP_SENSOR_DEVICE_ID);
	i2c_bus_sim_set_reg(bus, TEMP_SENSOR_REG_CONFIG, TEMP_SENSOR_CFG_DATA_READY);
	i2c_bus_sim_set_reg(bus, TEMP_SENSOR_REG_TEMP, 3200); /* 3200 * 7.8125 m°C = 25 °C */
}

int main(void)
{
	static struct i2c_bus bus;
	static struct temp_sensor sensor;
	static struct uart console;
	static struct app app;
	int32_t mdeg = 0;
	size_t pending = 0;
	int ret;

	/* Lỗi lúc khởi động thì không chạy tiếp được: dừng sớm, báo rõ lý do */
	if (i2c_bus_init(&bus, "I2C1") < 0 || uart_init(&console, "console", 115200) < 0) {
		printf("board init failed\n");
		return 1;
	}
	sim_sensor_ok(&bus);

	printf("== 1. Normal path ==\n");
	ret = temp_sensor_init(&sensor, &bus, SENSOR_ADDR);
	show("temp_sensor_init(0x48)", ret);
	if (ret < 0 || app_init(&app, &sensor, &console) < 0) {
		return 1;
	}
	show("app_report_temperature()", app_report_temperature(&app));
	i2c_bus_sim_set_reg(&bus, TEMP_SENSOR_REG_TEMP, (uint16_t)-80); /* -0.625 °C */
	show("app_report_temperature() below zero", app_report_temperature(&app));

	printf("\n== 2. Sensor not ready (-EAGAIN, handled by retry) ==\n");
	i2c_bus_sim_set_reg(&bus, TEMP_SENSOR_REG_CONFIG, 0);
	show("app_report_temperature()", app_report_temperature(&app));
	sim_sensor_ok(&bus);

	printf("\n== 3. Bus errors are translated, not forwarded ==\n");
	i2c_bus_sim_fail_next(&bus, -ETIMEDOUT, 1);
	show("temp_sensor_read() while bus says ETIMEDOUT", temp_sensor_read(&sensor, &mdeg));
	i2c_bus_sim_fail_next(&bus, -ENXIO, 1);
	show("temp_sensor_read() while bus says ENXIO", temp_sensor_read(&sensor, &mdeg));
	i2c_bus_sim_fail_next(&bus, -ETIMEDOUT, 1);
	show("app_report_temperature()", app_report_temperature(&app));

	printf("\n== 4. Init failures ==\n");
	show("temp_sensor_init(0x49): nobody ACKs", temp_sensor_init(&sensor, &bus, 0x49));
	i2c_bus_sim_set_reg(&bus, TEMP_SENSOR_REG_DEVICE_ID, 0x1234);
	show("temp_sensor_init(0x48): wrong chip ID", temp_sensor_init(&sensor, &bus, SENSOR_ADDR));
	sim_sensor_ok(&bus);
	i2c_bus_sim_fail_next(&bus, -ETIMEDOUT, 1);
	show("temp_sensor_init(0x48): bus stuck", temp_sensor_init(&sensor, &bus, SENSOR_ADDR));
	show("temp_sensor_init(0x48): bus ok again", temp_sensor_init(&sensor, &bus, SENSOR_ADDR));

	printf("\n== 5. Invalid arguments vs. real zero ==\n");
	show("temp_sensor_read(NULL, &mdeg)", temp_sensor_read(NULL, &mdeg));
	show("uart_tx_pending(NULL, &pending)", uart_tx_pending(NULL, &pending));
	ret = uart_tx_pending(&console, &pending);
	show("uart_tx_pending(&console, &pending)", ret);
	printf("%-44s    pending = %u\n", "", (unsigned)pending);

	printf("\n== 6. Positive value = partial success ==\n");
	const char *msg = "boot: sensor=TMP117 addr=0x48 ok\n";

	ret = uart_write(&console, (const uint8_t *)msg, strlen(msg));
	printf("%-44s -> %4d (%d of %u bytes accepted)\n", "uart_write(33 bytes) into 24-byte buffer",
	       ret, ret, (unsigned)strlen(msg));
	show("uart_write(again) while buffer full", uart_write(&console, (const uint8_t *)msg, 1));
	if (uart_flush(&console) < 0) {
		return 1;
	}
	show("app_console_write_all(33 bytes)", app_console_write_all(&app, msg, strlen(msg)));

	return 0;
}
