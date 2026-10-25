#ifndef RX_RING_H
#define RX_RING_H

#include "spinlock.h"

#include <stddef.h>
#include <stdint.h>

#define RX_RING_SIZE 16

/*
 * Ring buffer nhận UART: ISR ghi byte vào (rx_ring_put), main loop đọc ra
 * (rx_ring_get). Field count bị CẢ HAI phía sửa, nên mọi truy cập vào ring
 * phải nằm trong spinlock của chính ring.
 */
struct rx_ring {
	uint8_t buf[RX_RING_SIZE];
	size_t head;      /* vị trí ISR sẽ ghi tiếp */
	size_t tail;      /* vị trí main sẽ đọc tiếp */
	size_t count;     /* số byte đang nằm trong ring */
	uint32_t overrun; /* số byte phải bỏ vì ring đầy */
	struct spinlock lock;
};

int rx_ring_init(struct rx_ring *self);

/* Gọi từ ISR. Trả -ENOSPC nếu ring đầy (byte bị bỏ, overrun tăng) */
int rx_ring_put(struct rx_ring *self, uint8_t byte);

/* Gọi từ main. Trả -EAGAIN nếu ring rỗng */
int rx_ring_get(struct rx_ring *self, uint8_t *out);

size_t rx_ring_count(struct rx_ring *self);
uint32_t rx_ring_overrun(struct rx_ring *self);

/*
 * CHỈ DÙNG CHO DEMO: cùng logic nhưng KHÔNG lấy spinlock, để thấy race.
 * Firmware thật không bao giờ có cặp hàm này.
 */
int rx_ring_put_nolock(struct rx_ring *self, uint8_t byte);
int rx_ring_get_nolock(struct rx_ring *self, uint8_t *out);

#endif /* RX_RING_H */
