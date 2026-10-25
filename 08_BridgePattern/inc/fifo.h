#ifndef FIFO_H
#define FIFO_H

#include <stddef.h>
#include <stdint.h>

/*
 * Hàng đợi byte dùng chung cho các driver. Đây là Object Pattern thường
 * (struct trong header) vì struct của driver phải chứa nó trực tiếp. Mỗi
 * driver "kế thừa" nó bằng cách chứa một struct fifo bên trong (Inheritance
 * Pattern: containment), thay vì mỗi driver tự viết lại logic buffer.
 */
#define FIFO_SIZE 32

struct fifo {
	uint8_t buf[FIFO_SIZE];
	size_t count;
};

void fifo_init(struct fifo *self);
/* Trả về số byte thật sự đưa vào được (ít hơn len nếu đầy) */
size_t fifo_put(struct fifo *self, const uint8_t *data, size_t len);
/* Trả về số byte thật sự lấy ra được (0 nếu rỗng) */
size_t fifo_get(struct fifo *self, uint8_t *data, size_t len);

#endif /* FIFO_H */
