#ifndef BUF_POOL_H
#define BUF_POOL_H

#include "sem.h"
#include "spinlock.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define RX_BUF_SIZE 32

#ifndef BUF_POOL_COUNT
#define BUF_POOL_COUNT 3
#endif

struct rx_buf {
	uint8_t data[RX_BUF_SIZE];
	size_t len;
};

/*
 * Pool buffer tĩnh có khóa, tiếp nối pool của bài 02:
 *   - counting semaphore free đếm số buffer rảnh (ai muốn mượn phải take được),
 *   - spinlock lock bảo vệ mảng used[] (dữ liệu dùng chung).
 * Semaphore lo "còn hay hết", spinlock lo "slot nào", đúng lời khuyên của sách:
 * spinlock cho dữ liệu, semaphore cho báo hiệu/đếm tài nguyên.
 */
struct buf_pool {
	struct rx_buf bufs[BUF_POOL_COUNT];
	bool used[BUF_POOL_COUNT];
	struct spinlock lock;
	struct sem free;
	uint32_t in_use;     /* thống kê cho demo */
	uint32_t max_in_use;
};

int buf_pool_init(struct buf_pool *self);

/* ISR phải dùng K_NO_WAIT (nhận -EBUSY nếu hết buffer); thread có thể chờ */
int buf_pool_get(struct buf_pool *self, struct rx_buf **out, int32_t timeout_ms);

/* Trả buffer về pool và đặt *buf = NULL; trả -EINVAL nếu buffer không thuộc pool */
int buf_pool_put(struct buf_pool *self, struct rx_buf **buf);

uint32_t buf_pool_max_in_use(struct buf_pool *self);

#endif /* BUF_POOL_H */
