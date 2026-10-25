#include "text_logger.h"
#include "container_of.h"
#include "logger_ops.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

static char text_logger_level_char(enum log_level level)
{
	static const char names[] = { 'E', 'W', 'I', 'D' };

	return names[level];
}

static int text_logger_log(logger_t logger, enum log_level level, const char *msg)
{
	struct text_logger *self = CONTAINER_OF(logger, struct text_logger, logger);
	char line[TEXT_LOGGER_LINE_MAX];
	int len;
	int ret;

	/* Lọc ở phía abstraction: bản tin bị lọc không bao giờ đi qua cầu */
	if (level > self->max_level) {
		return 0;
	}

	self->seq++;
	len = snprintf(line, sizeof(line), "%lu [%c] %s\r\n", (unsigned long)self->seq,
		       text_logger_level_char(level), msg);
	if (len < 0 || (size_t)len >= sizeof(line)) {
		self->dropped++;
		return -EMSGSIZE;
	}

	/* Đi qua cầu: chỉ dùng generic API của hierarchy serial */
	ret = serial_write(self->out, (const uint8_t *)line, (size_t)len);
	if (ret < 0) {
		self->dropped++;
		return ret;
	}
	if (ret != len) {
		self->dropped++;
		return -EIO;
	}
	return 0;
}

static const struct logger_ops text_logger_ops = {
	.log = text_logger_log,
};

int text_logger_init(struct text_logger *self, serial_t out, enum log_level max_level)
{
	if (!self || !out) {
		return -EINVAL;
	}

	memset(self, 0, sizeof(*self));
	self->logger = &text_logger_ops;
	self->out = out;
	self->max_level = max_level;
	return 0;
}

logger_t text_logger_to_logger(struct text_logger *self)
{
	return self ? &self->logger : NULL;
}

uint32_t text_logger_dropped(const struct text_logger *self)
{
	return self ? self->dropped : 0;
}
