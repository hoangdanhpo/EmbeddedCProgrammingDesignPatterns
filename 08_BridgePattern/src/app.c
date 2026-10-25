#include "app.h"

#include <stdio.h>

int app_report(logger_t log, uint32_t uptime_s, int32_t temp_mdeg)
{
	char msg[32];
	enum log_level level = temp_mdeg > 70000 ? LOG_WRN : LOG_INF;
	/* Firmware tránh số thực: tách dấu, phần nguyên và một chữ số lẻ từ mili-độ */
	const char *sign = temp_mdeg < 0 ? "-" : "";
	uint32_t mag = temp_mdeg < 0 ? (uint32_t)(-(int64_t)temp_mdeg) : (uint32_t)temp_mdeg;

	snprintf(msg, sizeof(msg), "t=%lu T=%s%lu.%luC", (unsigned long)uptime_s, sign,
		 (unsigned long)(mag / 1000), (unsigned long)((mag % 1000) / 100));
	return logger_log(log, level, msg);
}
