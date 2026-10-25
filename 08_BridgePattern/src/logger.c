#include "logger.h"
#include "logger_ops.h"

#include <errno.h>

int logger_log(logger_t logger, enum log_level level, const char *msg)
{
	if (!logger || !*logger || !msg || level > LOG_DBG) {
		return -EINVAL;
	}
	if (!(*logger)->log) {
		return -ENOSYS;
	}
	return (*logger)->log(logger, level, msg);
}
