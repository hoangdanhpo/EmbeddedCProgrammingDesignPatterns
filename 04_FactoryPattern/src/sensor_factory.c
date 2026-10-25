#include "sensor_factory.h"
#include "fake_sensor.h"
#include "lm35.h"
#include "sensor_api.h"
#include "tmp102.h"

#include <errno.h>
#include <stdbool.h>
#include <string.h>

#ifndef SENSOR_FACTORY_MAX
#define SENSOR_FACTORY_MAX 4
#endif

/*
 * Một slot đủ chỗ cho cảm biến LỚN NHẤT trong các loại factory biết tạo.
 * Đây là lý do factory phải include header private của mọi driver: nó là nơi
 * DUY NHẤT trong chương trình cần biết kích thước của từng loại.
 */
union sensor_storage {
	struct tmp102 tmp102;
	struct lm35 lm35;
	struct fake_sensor fake;
};

struct sensor_slot {
	bool used;
	sensor_t handle; /* handle đã trao cho caller: destroy tìm slot bằng phép so sánh bằng */
	union sensor_storage obj;
};

/* Pool tĩnh trong .bss: RAM dùng cho cảm biến biết chính xác lúc build */
static struct sensor_slot sensor_slots[SENSOR_FACTORY_MAX];

static struct sensor_slot *sensor_slot_alloc(void)
{
	for (size_t i = 0; i < SENSOR_FACTORY_MAX; i++) {
		if (!sensor_slots[i].used) {
			sensor_slots[i].used = true;
			return &sensor_slots[i];
		}
	}
	return NULL;
}

static void sensor_slot_free(struct sensor_slot *slot)
{
	memset(slot, 0, sizeof(*slot)); /* xóa dấu vết object cũ, used = false, handle = NULL */
}

/*
 * Toàn bộ "kiến thức" về cách tạo từng loại nằm trong switch này. Thêm loại
 * mới = thêm một case ở đây; code ứng dụng không phải sửa gì.
 */
static int sensor_construct(struct sensor_slot *slot, const struct sensor_config *cfg,
			    struct sensor **out_base)
{
	int ret;

	switch (cfg->type) {
	case SENSOR_TYPE_TMP102:
		ret = tmp102_init(&slot->obj.tmp102, cfg->name, cfg->params.tmp102.i2c_addr);
		*out_base = &slot->obj.tmp102.base;
		break;
	case SENSOR_TYPE_LM35:
		ret = lm35_init(&slot->obj.lm35, cfg->name, cfg->params.lm35.adc_channel,
				cfg->params.lm35.vref_mv);
		*out_base = &slot->obj.lm35.base;
		break;
	case SENSOR_TYPE_FAKE:
		ret = fake_sensor_init(&slot->obj.fake, cfg->name, cfg->params.fake.value_milli_c);
		*out_base = &slot->obj.fake.base;
		break;
	default:
		ret = -EINVAL; /* loại chưa biết: dữ liệu cấu hình sai hoặc firmware cũ hơn cấu hình */
		break;
	}
	return ret;
}

int sensor_create(const struct sensor_config *cfg, sensor_t *out)
{
	struct sensor_slot *slot;
	struct sensor *base = NULL;
	int ret;

	if (!cfg || !cfg->name || !out) {
		return -EINVAL;
	}

	slot = sensor_slot_alloc();
	if (!slot) {
		return -ENOMEM;
	}

	ret = sensor_construct(slot, cfg, &base);
	if (ret) {
		sensor_slot_free(slot); /* tạo hỏng thì trả slot ngay, không để rò rỉ pool */
		return ret;
	}

	slot->handle = &base->api;
	*out = slot->handle;
	return 0;
}

int sensor_destroy(sensor_t *sensor)
{
	if (!sensor || !*sensor) {
		return -EINVAL;
	}

	for (size_t i = 0; i < SENSOR_FACTORY_MAX; i++) {
		struct sensor_slot *slot = &sensor_slots[i];

		if (slot->used && slot->handle == *sensor) {
			/* Factory chịu trách nhiệm gọi cleanup, rồi mới trả slot về pool */
			if ((**sensor)->deinit) {
				(**sensor)->deinit(*sensor);
			}
			sensor_slot_free(slot);
			*sensor = NULL;
			return 0;
		}
	}
	return -EINVAL; /* handle không do factory này tạo, hoặc đã destroy rồi */
}

size_t sensor_factory_free_slots(void)
{
	size_t n = 0;

	for (size_t i = 0; i < SENSOR_FACTORY_MAX; i++) {
		n += !sensor_slots[i].used;
	}
	return n;
}
