#ifndef FRAME_LOGGER_H
#define FRAME_LOGGER_H

#include "logger.h"
#include "serial.h"

#include <stddef.h>
#include <stdint.h>

/* Khung: [0xA5][len][level][payload: len byte][crc8 của len..payload] */
#define FRAME_LOGGER_SOF 0xA5u
#define FRAME_LOGGER_MAX_PAYLOAD 24

/*
 * Logger nhị phân gọn, cho tool trên PC giải mã (tiết kiệm băng thông, có CRC).
 * Cùng interface logger với text_logger, cùng cách giữ cầu sang serial_t, nhưng
 * định dạng khác hẳn. Thêm logger này không phải sửa một dòng nào của các
 * driver serial, và ngược lại.
 */
struct frame_logger {
	const struct logger_ops *logger;
	serial_t out;
	uint32_t dropped;
};

int frame_logger_init(struct frame_logger *self, serial_t out);
logger_t frame_logger_to_logger(struct frame_logger *self);
uint32_t frame_logger_dropped(const struct frame_logger *self);
/* Dùng chung cho bên giải mã (test, tool): CRC-8 đa thức 0x07, giá trị đầu 0 */
uint8_t frame_logger_crc8(const uint8_t *data, size_t len);

#endif /* FRAME_LOGGER_H */
