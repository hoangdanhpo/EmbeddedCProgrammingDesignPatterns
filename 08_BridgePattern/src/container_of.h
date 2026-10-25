#ifndef CONTAINER_OF_H
#define CONTAINER_OF_H

#include <stddef.h>

/*
 * Từ con trỏ tới một member, tính ra con trỏ tới struct bao ngoài nó:
 * địa chỉ struct = địa chỉ member - offset của member trong struct.
 *
 * Vế sizeof(...) không sinh ra lệnh máy nào (sizeof không tính biểu thức bên
 * trong). Nó chỉ bắt compiler so sánh kiểu của ptr với kiểu của &member: truyền
 * nhầm con trỏ thì gcc báo "comparison of distinct pointer types", và -Werror
 * biến nó thành lỗi build thay vì bug lúc chạy.
 *
 * Đi qua (void *) để gcc không cảnh báo về căn lề khi ép từ char *.
 */
#define CONTAINER_OF(ptr, type, member)                                                      \
	((void)sizeof((ptr) == &((type *)0)->member),                                            \
	 (type *)(void *)((char *)(ptr) - offsetof(type, member)))

#endif /* CONTAINER_OF_H */
