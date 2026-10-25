#ifndef BUF_QUEUE_H
#define BUF_QUEUE_H

#include "buf_pool.h"
#include "sem.h"
#include "spinlock.h"

#include <stddef.h>
#include <stdint.h>

#define BUF_QUEUE_LEN 4

/*
 * Hàng đợi các buffer đã có dữ liệu, chờ thread xử lý: một message queue thu nhỏ.
 *   - ring items[] nằm trong spinlock (dữ liệu dùng chung giữa ISR và thread),
 *   - semaphore ready báo "có việc": ISR give, worker take.
 */
struct buf_queue {
	struct rx_buf *items[BUF_QUEUE_LEN];
	size_t head;
	size_t tail;
	size_t count;
	struct spinlock lock;
	struct sem ready;
};

int buf_queue_init(struct buf_queue *self);

/* Gọi được từ ISR, không bao giờ block; -ENOSPC nếu đầy. buf = NULL là tin "dừng" */
int buf_queue_put(struct buf_queue *self, struct rx_buf *buf);

/* Chỉ gọi từ thread; chờ tới khi có buffer hoặc hết timeout_ms */
int buf_queue_get(struct buf_queue *self, struct rx_buf **out, int32_t timeout_ms);

#endif /* BUF_QUEUE_H */
