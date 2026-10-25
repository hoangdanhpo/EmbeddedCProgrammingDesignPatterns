#ifndef SAMPLE_QUEUE_H
#define SAMPLE_QUEUE_H

#include "condvar.h"
#include "mutex.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SAMPLE_QUEUE_CAP 4

/*
 * Hàng đợi mẫu ADC có giới hạn: producer chờ khi đầy, consumer chờ khi rỗng.
 * Hai điều kiện khác nhau nên hai condvar, nhưng dùng CHUNG một mutex
 * vì cả hai điều kiện cùng đọc count.
 */
struct sample_queue {
	struct mutex lock;
	struct condvar not_empty;
	struct condvar not_full;
	int32_t buf[SAMPLE_QUEUE_CAP]; /* Mọi field từ đây trở xuống được bảo vệ bởi lock */
	size_t head;
	size_t count;
	bool closed;
};

int sample_queue_init(struct sample_queue *self);
int sample_queue_deinit(struct sample_queue *self);

/* 0; -EAGAIN nếu hết giờ khi đầy; -EPIPE nếu hàng đợi đã đóng. */
int sample_queue_put(struct sample_queue *self, int32_t value, int32_t timeout_ms);

/* 0; -EAGAIN nếu hết giờ khi rỗng; -EPIPE nếu đã đóng VÀ đã lấy hết. */
int sample_queue_get(struct sample_queue *self, int32_t *value, int32_t timeout_ms);

/* Đóng hàng đợi và đánh thức mọi thread đang chờ để chúng thoát ra. */
int sample_queue_close(struct sample_queue *self);

#endif /* SAMPLE_QUEUE_H */
