#ifndef LOGGER_OPS_H
#define LOGGER_OPS_H

#include "logger.h"

/*
 * Bảng hàm ảo của hierarchy logger ("project private", chỉ logger.c và các
 * logger cụ thể include). Cùng khuôn Virtual API với serial_ops.h của bài 07.
 */
struct logger_ops {
	int (*log)(logger_t logger, enum log_level level, const char *msg);
};

#endif /* LOGGER_OPS_H */
