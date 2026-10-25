#include "sensor.h"
#include "sensor_api.h"

#include <errno.h>

/*
 * Lớp "chuyển tiếp" (delegator): thấy được định nghĩa struct sensor_api nên
 * gọi được hàm ảo. Ứng dụng chỉ gọi hai hàm này, không bao giờ gọi thẳng api.
 */
int sensor_read(sensor_t sensor, int32_t *out_milli_c)
{
	if (!sensor || !*sensor || !out_milli_c) {
		return -EINVAL;
	}

	return (*sensor)->read(sensor, out_milli_c);
}

const char *sensor_name(sensor_t sensor)
{
	if (!sensor) {
		return "(null)";
	}

	/* Mọi cảm biến cụ thể đều nhúng struct sensor, nên lấy được phần chung mà không cần biết loại */
	return CONTAINER_OF(sensor, struct sensor, api)->name;
}
