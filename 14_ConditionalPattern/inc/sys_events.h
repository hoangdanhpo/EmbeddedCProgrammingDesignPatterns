#ifndef SYS_EVENTS_H
#define SYS_EVENTS_H

#include "condvar.h"
#include "mutex.h"

#include <stdint.h>

/* Các mốc khởi động của thiết bị; mỗi mốc một bit */
#define SYS_EVT_CONFIG_LOADED (1u << 0) /* Đã đọc cấu hình từ flash */
#define SYS_EVT_NET_UP (1u << 1)        /* Có IP (DHCP xong) */
#define SYS_EVT_TIME_SYNCED (1u << 2)   /* Đã đồng bộ giờ (NTP) */

/*
 * "Cổng khởi động": nhiều thread chờ các tổ hợp mốc khác nhau, một thread
 * khác đánh dấu mốc và đánh thức TẤT CẢ để mỗi thread tự kiểm tra điều kiện của mình.
 * Điều kiện (bits) và condvar luôn đi cùng một mutex, cả ba nằm trong một object.
 */
struct sys_events {
	struct mutex lock;
	struct condvar changed;
	uint32_t bits;   /* Được bảo vệ bởi lock */
};

int sys_events_init(struct sys_events *self);
int sys_events_deinit(struct sys_events *self);

/* Bật các bit và đánh thức mọi thread đang chờ. Trả về số thread được đánh thức. */
int sys_events_set(struct sys_events *self, uint32_t bits);

/*
 * Chờ tới khi MỌI bit trong mask đều bật. timeout_ms là tổng thời gian chờ
 * (MUTEX_FOREVER hoặc >= 0). Trả về 0 hoặc -EAGAIN.
 * wakeups (có thể NULL): số lần bị đánh thức, để demo/test thấy vòng lặp kiểm tra lại.
 */
int sys_events_wait_all(struct sys_events *self, uint32_t mask, int32_t timeout_ms, uint32_t *wakeups);

#endif /* SYS_EVENTS_H */
