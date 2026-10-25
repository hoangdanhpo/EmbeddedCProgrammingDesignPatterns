#ifndef MEM_SERIAL_H
#define MEM_SERIAL_H

#include "fifo.h"
#include "serial.h"

#include <stddef.h>
#include <stdint.h>

/*
 * Cổng serial giả nằm hoàn toàn trong RAM: thứ được "gửi" đi bị giữ lại
 * trong tx để test đọc ra kiểm tra; thứ test muốn "nhận" thì nạp vào rx.
 * Nhờ interface serial, module như gps.c được test trên PC mà không cần
 * UART thật và không phải sửa một dòng nào.
 */
struct mem_serial {
	const struct serial_ops *serial;
	struct fifo tx;
	struct fifo rx;
};

int mem_serial_init(struct mem_serial *self);
serial_t mem_serial_to_serial(struct mem_serial *self);
/* Lấy ra (và xóa) những gì đã được ghi vào cổng */
size_t mem_serial_take_tx(struct mem_serial *self, uint8_t *out, size_t cap);
/* Nạp dữ liệu để serial_read đọc ra */
void mem_serial_inject_rx(struct mem_serial *self, const uint8_t *data, size_t len);

#endif /* MEM_SERIAL_H */
