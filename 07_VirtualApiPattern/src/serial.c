#include "serial.h"
#include "serial_ops.h"

#include <errno.h>

/*
 * Generic API: lớp "delegator" mỏng. File này là nơi duy nhất ngoài các
 * implementation thấy định nghĩa struct serial_ops, nên nó có thể gọi qua
 * bảng hàm. Kiểm tra tham số đặt ở đây một lần, mọi implementation được
 * hưởng, thay vì mỗi implementation tự lặp lại (và có cái quên).
 */

int serial_write(serial_t serial, const uint8_t *data, size_t len)
{
	if (!serial || !*serial || (!data && len > 0)) {
		return -EINVAL;
	}
	if (!(*serial)->write) {
		return -ENOSYS;
	}
	/* Lần gián tiếp thứ hai: nhảy vào hàm của implementation cụ thể */
	return (*serial)->write(serial, data, len);
}

int serial_read(serial_t serial, uint8_t *data, size_t len)
{
	if (!serial || !*serial || (!data && len > 0)) {
		return -EINVAL;
	}
	if (!(*serial)->read) {
		return -ENOSYS;
	}
	return (*serial)->read(serial, data, len);
}
