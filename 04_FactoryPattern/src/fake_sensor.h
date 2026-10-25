#ifndef FAKE_SENSOR_H
#define FAKE_SENSOR_H

#include <stdint.h>

#include "sensor_api.h"

struct fake_sensor {
	struct sensor base; /* phải là field đầu tiên */
	int32_t value_milli_c;
	uint32_t reads; /* đếm số lần đọc: test kiểm tra được code ứng dụng đã đọc bao nhiêu lần */
};

int fake_sensor_init(struct fake_sensor *self, const char *name, int32_t value_milli_c);

#endif /* FAKE_SENSOR_H */
