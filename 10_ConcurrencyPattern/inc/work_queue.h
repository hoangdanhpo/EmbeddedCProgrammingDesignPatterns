#ifndef WORK_QUEUE_H
#define WORK_QUEUE_H

#include <pthread.h>
#include <stdbool.h>

/*
 * Work item: một "việc cần làm" nằm sẵn bên trong object sở hữu nó
 * (callback pattern + CONTAINER_OF). Không cấp phát động khi submit.
 */
struct work {
	void (*handler)(struct work *work);
	struct work *next;
	bool pending; /* đang nằm trong hàng đợi, chờ được chạy */
};

/*
 * Hàng đợi công việc chạy trên MỘT thread riêng. Các work item chạy lần lượt,
 * không bao giờ chen nhau, nên handler không cần khóa lẫn nhau. Đây là bản
 * thu nhỏ của "system work queue" trong Zephyr (k_work_submit).
 *
 * mutex và condition variable bên dưới là primitive của bài 13 và 14; ở bài
 * này chỉ dùng như hộp đen để thread worker ngủ khi không có việc.
 */
struct work_queue {
	pthread_t thread;
	pthread_mutex_t lock;
	pthread_cond_t cond;
	struct work *head;
	struct work *tail;
	bool running;
	unsigned executed;
};

/* Trả về 0 hoặc -EINVAL */
int work_init(struct work *self, void (*handler)(struct work *work));

/* Tạo thread worker. Trả về 0, -EINVAL, hoặc -EAGAIN nếu không tạo được thread */
int work_queue_start(struct work_queue *self);

/*
 * Đưa work vào cuối hàng đợi. Gọi được từ bất kỳ thread nào.
 *
 * Trả về:
 *   0        đã xếp vào hàng đợi
 *   1        work đang chờ sẵn trong hàng đợi rồi, không xếp thêm lần nữa
 *   -EINVAL  tham số sai hoặc hàng đợi chưa chạy
 */
int work_queue_submit(struct work_queue *self, struct work *work);

/* Chạy nốt các work còn trong hàng đợi rồi dừng thread. Trả về 0 hoặc -EINVAL */
int work_queue_stop(struct work_queue *self);

#endif /* WORK_QUEUE_H */
