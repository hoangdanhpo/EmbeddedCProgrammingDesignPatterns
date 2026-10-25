#ifndef MUTEX_H
#define MUTEX_H

#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>

/* Giá trị đặc biệt cho tham số timeout_ms, cùng ý nghĩa với K_NO_WAIT / K_FOREVER của Zephyr */
#define MUTEX_NO_WAIT 0
#define MUTEX_FOREVER (-1)

/*
 * Một phần tử trong hàng đợi của mutex. Mỗi thread phải chờ tự tạo một node
 * trên stack của nó, nên mutex không cần cấp phát gì thêm.
 */
struct mutex_waiter {
	pthread_t thread;
	pthread_cond_t wake;        /* Mỗi waiter một "chuông" riêng: đánh thức đúng người */
	bool granted;               /* Người unlock đã trao quyền giữ mutex cho waiter này */
	struct mutex_waiter *next;
};

/*
 * Mutex có chủ sở hữu (owner), cho phép owner khóa lồng (recursive), trao quyền
 * trực tiếp cho waiter đầu hàng đợi khi unlock (giống k_mutex của Zephyr).
 *
 * Struct để public (Object Pattern, không Opaque) có chủ ý: mutex phải nhúng
 * được thẳng vào object mà nó bảo vệ (struct i2c_bus { struct mutex lock; ... }),
 * giống struct k_mutex. Code bên ngoài chỉ được dùng qua API, không đọc/ghi field.
 *
 * Trên RTOS, các field dưới đây được bảo vệ bằng spinlock và thread chờ bị
 * scheduler suspend. Trên PC, guard đóng vai spinlock và pthread_cond trong
 * từng waiter đóng vai "suspend/ready" của scheduler.
 */
struct mutex {
	pthread_mutex_t guard;
	pthread_t owner;            /* Chỉ có nghĩa khi lock_count > 0 */
	uint32_t lock_count;        /* 0 = đang mở; > 1 = owner đã khóa lồng */
	struct mutex_waiter *head;  /* Hàng đợi FIFO các thread đang chờ */
	struct mutex_waiter *tail;
	uint32_t contentions;       /* Thống kê: số lần lock phải vào hàng đợi */
};

int mutex_init(struct mutex *self);
int mutex_deinit(struct mutex *self);

/*
 * timeout_ms: MUTEX_NO_WAIT, MUTEX_FOREVER, hoặc số mili giây > 0.
 * Trả về 0 khi đã giữ mutex; -EBUSY nếu NO_WAIT mà mutex bận;
 * -EAGAIN nếu hết thời gian chờ; -EINVAL nếu tham số sai.
 */
int mutex_lock(struct mutex *self, int32_t timeout_ms);

/* Trả về 0; -EINVAL nếu mutex đang mở; -EPERM nếu thread gọi không phải owner. */
int mutex_unlock(struct mutex *self);

/* Số lần thread hiện tại đang khóa lồng mutex (>= 1), hoặc -EPERM nếu không phải owner. */
int mutex_lock_depth(struct mutex *self);

#endif /* MUTEX_H */
