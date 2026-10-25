#ifndef SLOW_SENSOR_H
#define SLOW_SENSOR_H

#include <stdint.h>

/*
 * Cảm biến nhiệt "chậm", phỏng theo DS18B20 ở độ phân giải 11-bit: mỗi lần
 * đo phải chờ 375 ms mới có kết quả. Driver viết kiểu tuần tự đơn giản nhất:
 * ra lệnh đo, chờ, đọc. Trong lúc chờ, thread gọi hàm bị chặn.
 */
#define SLOW_SENSOR_CONVERSION_MS 375u

struct slow_sensor {
	uint32_t conversion_ms;
	int32_t sim_mdeg; /* nhiệt độ giả lập, tăng dần sau mỗi lần đo */
	uint32_t reads;
};

/* Trả về 0 hoặc -EINVAL */
int slow_sensor_init(struct slow_sensor *self, uint32_t conversion_ms);

/*
 * Chặn thread gọi trong conversion_ms rồi trả kết quả qua out_mdeg_c.
 * Trả về 0 hoặc -EINVAL.
 */
int slow_sensor_read(struct slow_sensor *self, int32_t *out_mdeg_c);

#endif /* SLOW_SENSOR_H */
