#ifndef SENSOR_API_H
#define SENSOR_API_H

#include <stddef.h>

#include "sensor.h"

/*
 * Header "project private": chỉ phần generic (sensor.c), các driver cụ thể và
 * factory được include. Ứng dụng không thấy file này (Makefile chỉ có -Iinc).
 */

/* Từ con trỏ tới một field, lùi lại đúng offset của field đó để ra con trỏ tới struct chứa nó */
#define CONTAINER_OF(ptr, type, member) ((type *)((char *)(ptr) - offsetof(type, member)))

/* Bảng hàm ảo: mỗi loại cảm biến có MỘT bảng const, nằm trong Flash */
struct sensor_api {
	int (*read)(sensor_t sensor, int32_t *out_milli_c);
	void (*deinit)(sensor_t sensor); /* có thể NULL nếu không cần dọn dẹp */
};

/*
 * Phần chung mà mọi cảm biến cụ thể nhúng vào (field đầu tiên).
 * Handle trao cho ứng dụng chính là &base.api.
 */
struct sensor {
	const struct sensor_api *api;
	const char *name;
};

#endif /* SENSOR_API_H */
