#ifndef APP_H
#define APP_H

#include "logger.h"

#include <stdint.h>

/*
 * Logic ứng dụng: chỉ biết logger_t. Không biết định dạng log, không biết
 * đường truyền. Đổi cả hai thứ đó không phải build lại app.c.
 */
int app_report(logger_t log, uint32_t uptime_s, int32_t temp_mdeg);

#endif /* APP_H */
