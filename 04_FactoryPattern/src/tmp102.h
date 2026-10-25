#ifndef TMP102_H
#define TMP102_H

#include <stdint.h>

#include "sensor_api.h"

/*
 * Header private: chỉ factory include, vì factory cần biết KÍCH THƯỚC struct
 * để dành chỗ trong pool. Ứng dụng không bao giờ thấy struct tmp102.
 */
struct tmp102 {
	struct sensor base; /* phải là field đầu tiên: handle = &base.api */
	uint8_t i2c_addr;
};

int tmp102_init(struct tmp102 *self, const char *name, uint8_t i2c_addr);

#endif /* TMP102_H */
