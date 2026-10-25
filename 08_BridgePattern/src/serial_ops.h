#ifndef SERIAL_OPS_H
#define SERIAL_OPS_H

#include "serial.h"

/*
 * API interface ("project private"): bảng hàm ảo của interface serial.
 * Chỉ serial.c và các implementation include file này. Nó nằm trong src/
 * chứ không nằm trong inc/, nên main.c (build với -Iinc) không include được.
 *
 * Mỗi implementation định nghĩa đúng MỘT bảng "static const struct serial_ops"
 * dùng chung cho mọi instance của nó. Bảng là const nên nằm trong Flash
 * (.rodata) và không ai sửa được lúc chạy.
 *
 * Mọi hàm nhận chính handle (serial_t), không nhận void *: implementation
 * lấy lại object của nó bằng CONTAINER_OF, compiler vẫn kiểm tra được kiểu.
 */
struct serial_ops {
	int (*write)(serial_t serial, const uint8_t *data, size_t len);
	int (*read)(serial_t serial, uint8_t *data, size_t len);
};

#endif /* SERIAL_OPS_H */
