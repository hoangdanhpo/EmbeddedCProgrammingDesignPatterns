#include "sensor.h"
#include "sensor_factory.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

/*
 * Mô tả phần cứng của board bằng DỮ LIỆU, không bằng code. Đổi board = sửa
 * bảng này (hoặc sinh nó từ device tree / file JSON lúc build).
 */
static const struct sensor_config board_sensors[] = {
	{ .type = SENSOR_TYPE_TMP102, .name = "board", .params.tmp102 = { .i2c_addr = 0x48 } },
	{ .type = SENSOR_TYPE_TMP102, .name = "outdoor", .params.tmp102 = { .i2c_addr = 0x49 } },
	{ .type = SENSOR_TYPE_LM35, .name = "motor",
	  .params.lm35 = { .adc_channel = 3, .vref_mv = 3300 } },
	/* Có trong cấu hình nhưng chip không được hàn trên bản board này */
	{ .type = SENSOR_TYPE_TMP102, .name = "spare", .params.tmp102 = { .i2c_addr = 0x4A } },
};

static void print_milli_c(const char *name, int32_t milli_c)
{
	/* Tự in phần nguyên và phần lẻ: printf("%f") kéo theo cả thư viện float trên MCU */
	printf("  %-8s %s%ld.%03ld C\n", name, milli_c < 0 ? "-" : "", labs((long)milli_c) / 1000,
	       labs((long)milli_c) % 1000);
}

/* Code này không biết và không cần biết cảm biến nào là TMP102 hay LM35 */
static void read_all(sensor_t *sensors, size_t count)
{
	for (size_t i = 0; i < count; i++) {
		int32_t milli_c;

		if (sensors[i] && sensor_read(sensors[i], &milli_c) == 0) {
			print_milli_c(sensor_name(sensors[i]), milli_c);
		}
	}
}

int main(void)
{
	sensor_t sensors[ARRAY_SIZE(board_sensors)] = { 0 };
	sensor_t extra[4] = { 0 };
	int ret;

	printf("create from board table (pool has %u slots):\n", (unsigned)sensor_factory_free_slots());
	for (size_t i = 0; i < ARRAY_SIZE(board_sensors); i++) {
		ret = sensor_create(&board_sensors[i], &sensors[i]);
		printf("  %-8s -> %d\n", board_sensors[i].name, ret);
	}

	printf("read all:\n");
	read_all(sensors, ARRAY_SIZE(sensors));

	/* Tham số sai bị chặn tại MỘT chỗ duy nhất: factory (qua init của driver) */
	const struct sensor_config bad_addr = { .type = SENSOR_TYPE_TMP102, .name = "bad",
						.params.tmp102 = { .i2c_addr = 0x50 } };
	ret = sensor_create(&bad_addr, &extra[0]);
	printf("tmp102 @ 0x50 -> %d (-EINVAL = %d)\n", ret, -EINVAL);

	printf("free slots: %u\n", (unsigned)sensor_factory_free_slots());
	const struct sensor_config fake = { .type = SENSOR_TYPE_FAKE, .name = "fake",
					    .params.fake = { .value_milli_c = 42000 } };
	ret = sensor_create(&fake, &extra[0]);
	printf("fake #1 -> %d\n", ret);
	ret = sensor_create(&fake, &extra[1]);
	printf("fake #2 -> %d (-ENOMEM = %d)\n", ret, -ENOMEM);
	read_all(extra, ARRAY_SIZE(extra));

	printf("destroy outdoor:\n");
	ret = sensor_destroy(&sensors[1]);
	printf("  -> %d, handle is %s, free slots: %u\n", ret, sensors[1] ? "NOT NULL" : "NULL",
	       (unsigned)sensor_factory_free_slots());
	ret = sensor_create(&fake, &extra[1]);
	printf("fake #2 again -> %d\n", ret);

	printf("destroy everything:\n");
	for (size_t i = 0; i < ARRAY_SIZE(sensors); i++) {
		sensor_destroy(&sensors[i]); /* handle NULL (tạo hỏng / đã hủy) trả -EINVAL, vô hại */
	}
	for (size_t i = 0; i < ARRAY_SIZE(extra); i++) {
		sensor_destroy(&extra[i]);
	}
	printf("free slots: %u\n", (unsigned)sensor_factory_free_slots());
	return 0;
}
