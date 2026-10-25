#include "frame_logger.h"
#include "container_of.h"
#include "logger_ops.h"

#include <errno.h>
#include <string.h>

uint8_t frame_logger_crc8(const uint8_t *data, size_t len)
{
	uint8_t crc = 0;

	for (size_t i = 0; i < len; i++) {
		crc ^= data[i];
		for (int bit = 0; bit < 8; bit++) {
			crc = (crc & 0x80u) ? (uint8_t)((crc << 1) ^ 0x07u) : (uint8_t)(crc << 1);
		}
	}
	return crc;
}

static int frame_logger_log(logger_t logger, enum log_level level, const char *msg)
{
	struct frame_logger *self = CONTAINER_OF(logger, struct frame_logger, logger);
	uint8_t frame[FRAME_LOGGER_MAX_PAYLOAD + 4];
	size_t n = strlen(msg);
	size_t total;
	int ret;

	if (n > FRAME_LOGGER_MAX_PAYLOAD) {
		self->dropped++;
		return -EMSGSIZE;
	}

	frame[0] = FRAME_LOGGER_SOF;
	frame[1] = (uint8_t)n;
	frame[2] = (uint8_t)level;
	memcpy(&frame[3], msg, n);
	/* CRC phủ len, level và payload: bên nhận phát hiện được byte hỏng trên đường truyền */
	frame[3 + n] = frame_logger_crc8(&frame[1], n + 2);
	total = n + 4;

	ret = serial_write(self->out, frame, total);
	if (ret < 0) {
		self->dropped++;
		return ret;
	}
	if ((size_t)ret != total) {
		self->dropped++;
		return -EIO;
	}
	return 0;
}

static const struct logger_ops frame_logger_ops = {
	.log = frame_logger_log,
};

int frame_logger_init(struct frame_logger *self, serial_t out)
{
	if (!self || !out) {
		return -EINVAL;
	}

	memset(self, 0, sizeof(*self));
	self->logger = &frame_logger_ops;
	self->out = out;
	return 0;
}

logger_t frame_logger_to_logger(struct frame_logger *self)
{
	return self ? &self->logger : NULL;
}

uint32_t frame_logger_dropped(const struct frame_logger *self)
{
	return self ? self->dropped : 0;
}
