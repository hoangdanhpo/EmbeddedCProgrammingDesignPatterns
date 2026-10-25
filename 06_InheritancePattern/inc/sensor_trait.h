#ifndef SENSOR_TRAIT_H
#define SENSOR_TRAIT_H

#include <errno.h>
#include <stddef.h>
#include <stdint.h>

/*
 * Trait "đọc được một giá trị đo". Chỉ gồm con trỏ hàm. Kiểu nào nhúng trait
 * này và gán read thì được đối xử như "một cảm biến" bởi code dùng chung.
 */
struct sensor_trait {
	int (*read)(struct sensor_trait *self, int32_t *value);
};

/* Hàm wrapper chung: caller không phải tự gọi qua con trỏ hàm */
static inline int sensor_trait_read(struct sensor_trait *self, int32_t *value)
{
	if (!self || !value) {
		return -EINVAL;
	}
	if (!self->read) {
		return -ENOSYS; /* kiểu dẫn xuất chưa cài hành vi này */
	}
	return self->read(self, value);
}

#endif /* SENSOR_TRAIT_H */
