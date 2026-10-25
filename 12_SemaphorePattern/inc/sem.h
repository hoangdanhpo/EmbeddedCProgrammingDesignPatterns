#ifndef SEM_H
#define SEM_H

#include "kthread.h"

#include <stdint.h>

/*
 * Counting semaphore theo đúng thuật toán k_sem của Zephyr mà sách trích:
 *   give: có thread đang chờ -> đánh thức thread ưu tiên cao nhất, count GIỮ NGUYÊN
 *         không ai chờ        -> count++ (không vượt limit)
 *   take: count > 0           -> count--, trả 0 ngay
 *         count == 0          -> K_NO_WAIT: -EBUSY; còn lại: vào wait queue và ngủ
 *
 * count = số "tài nguyên" đang rảnh, KHÔNG phải số thread đang chờ.
 * Struct public (Object Pattern) để nhúng được vào object khác.
 */
struct sem {
	uint32_t count;
	uint32_t limit;
	struct wait_q wait_q;
};

#define SEM_MAX_LIMIT UINT32_MAX

/* limit > 0 và initial_count <= limit, nếu không trả -EINVAL */
int sem_init(struct sem *self, uint32_t initial_count, uint32_t limit);

/* Gọi được từ thread và từ ISR; không bao giờ block */
int sem_give(struct sem *self);

/*
 * timeout_ms: K_NO_WAIT, K_FOREVER hoặc số mili giây.
 * Trả 0 khi lấy được, -EBUSY nếu K_NO_WAIT mà không lấy được, -EAGAIN khi hết
 * giờ, -EINVAL nếu gọi từ ISR với timeout khác K_NO_WAIT (ISR không được ngủ).
 */
int sem_take(struct sem *self, int32_t timeout_ms);

uint32_t sem_count_get(struct sem *self);

#endif /* SEM_H */
