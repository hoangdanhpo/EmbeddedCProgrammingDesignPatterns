#include "sensor_task.h"
#include "container_of.h"

#include <errno.h>

/* Chạy trên thread của work queue, KHÔNG phải thread chính */
static void sensor_task_handler(struct work *work)
{
	struct sensor_task *self = CONTAINER_OF(work, struct sensor_task, work);
	int32_t mdeg;

	if (slow_sensor_read(self->sensor, &mdeg) < 0) {
		atomic_fetch_add(&self->errors, 1u);
		return;
	}
	/* Ghi giá trị trước, tăng bộ đếm sau: ai thấy samples > 0 thì đã có giá trị */
	atomic_store(&self->latest_mdeg, mdeg);
	atomic_fetch_add(&self->samples, 1u);
}

int sensor_task_init(struct sensor_task *self, struct slow_sensor *sensor)
{
	if (!self || !sensor) {
		return -EINVAL;
	}

	self->sensor = sensor;
	atomic_init(&self->latest_mdeg, 0);
	atomic_init(&self->samples, 0u);
	atomic_init(&self->errors, 0u);
	return work_init(&self->work, sensor_task_handler);
}

int sensor_task_latest(struct sensor_task *self, int32_t *out_mdeg_c)
{
	if (!self || !out_mdeg_c) {
		return -EINVAL;
	}

	if (atomic_load(&self->samples) == 0) {
		return -EAGAIN;
	}
	*out_mdeg_c = atomic_load(&self->latest_mdeg);
	return 0;
}
