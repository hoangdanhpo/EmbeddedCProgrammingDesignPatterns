#ifndef HEARTBEAT_H
#define HEARTBEAT_H

#include <stdbool.h>
#include <stdint.h>

#define HEARTBEAT_MAX_SLOTS 64

/*
 * LED nhịp tim: phải đảo trạng thái đều đặn mỗi period_ms. Đây là "deadline"
 * mềm của hệ thống: người dùng nhìn LED giật là biết firmware đang bị kẹt.
 * Object còn tự đo độ trễ để demo in ra được, không chỉ "nhìn bằng mắt".
 */
struct heartbeat {
	uint32_t period_ms;
	uint32_t start_ms;
	uint32_t next_due_ms;
	uint32_t last_toggle_ms;
	bool led_on;
	uint32_t toggles;
	uint32_t max_interval_ms; /* khoảng cách lớn nhất giữa hai lần đảo LED */
	uint32_t late;            /* số lần đảo trễ hơn 1.5 chu kỳ */
	char timeline[HEARTBEAT_MAX_SLOTS + 1]; /* '#' = có đảo LED trong ô thời gian đó */
};

/* Trả về 0 hoặc -EINVAL */
int heartbeat_init(struct heartbeat *self, uint32_t period_ms, uint32_t now_ms);

/*
 * Gọi càng thường xuyên càng tốt. Đảo LED nếu đã tới hạn.
 * Trả về 1 nếu vừa đảo LED, 0 nếu chưa tới hạn, -EINVAL nếu tham số sai.
 */
int heartbeat_poll(struct heartbeat *self, uint32_t now_ms);

#endif /* HEARTBEAT_H */
