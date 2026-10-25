#ifndef CONTAINER_OF_H
#define CONTAINER_OF_H

#include <stddef.h>

/*
 * Từ địa chỉ của một member, lùi lại đúng offsetof(type, member) byte để ra
 * địa chỉ của struct bao ngoài. Đây là cách callback lấy lại "self" của
 * observer mà không cần biến global hay con trỏ void *user_data.
 *
 * Vế "0 * sizeof(...)" không sinh mã nào (sizeof không tính biểu thức bên
 * trong), nó chỉ bắt compiler so sánh kiểu của ptr với kiểu của member:
 * truyền nhầm con trỏ tới member khác kiểu sẽ bị báo
 * "comparison of distinct pointer types" (thành lỗi khi có -Werror).
 */
#define CONTAINER_OF(ptr, type, member) \
	((type *)((char *)(ptr) - offsetof(type, member) + \
		  0 * sizeof((ptr) == &((type *)0)->member)))

#endif /* CONTAINER_OF_H */
