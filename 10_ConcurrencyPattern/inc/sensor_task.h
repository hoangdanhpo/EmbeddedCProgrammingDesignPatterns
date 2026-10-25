#ifndef SENSOR_TASK_H
#define SENSOR_TASK_H

#include <stdatomic.h>
#include <stdint.h>

#include "slow_sensor.h"
#include "work_queue.h"

/*
 * Gói việc "đọc cảm biến chậm" thành một work item. Phần chậm chạy trên thread
 * của work queue; vòng lặp chính chỉ submit rồi đọc kết quả mới nhất.
 *
 * Hai field kết quả được ghi bởi thread worker và đọc bởi thread chính, nên là
 * biến atomic: mỗi lần đọc/ghi là một thao tác không thể bị cắt ngang.
 */
struct sensor_task {
	struct work work; /* member, không phải con trỏ: handler tìm lại object bằng CONTAINER_OF */
	struct slow_sensor *sensor;
	_Atomic int32_t latest_mdeg;
	atomic_uint samples;
	atomic_uint errors;
};

/* Trả về 0 hoặc -EINVAL */
int sensor_task_init(struct sensor_task *self, struct slow_sensor *sensor);

/*
 * Kết quả mới nhất, không chặn.
 * Trả về 0, -EINVAL, hoặc -EAGAIN nếu chưa có mẫu nào.
 */
int sensor_task_latest(struct sensor_task *self, int32_t *out_mdeg_c);

#endif /* SENSOR_TASK_H */
