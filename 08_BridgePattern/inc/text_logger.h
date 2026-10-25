#ifndef TEXT_LOGGER_H
#define TEXT_LOGGER_H

#include "logger.h"
#include "serial.h"

#include <stdint.h>

#define TEXT_LOGGER_LINE_MAX 64

/*
 * Logger dạng chữ cho người đọc trên terminal: "<seq> [I] <msg>\r\n".
 * Một implementation của hierarchy logger, và là đầu cầu bên này: nó giữ một
 * serial_t (handle của hierarchy bên kia) và chỉ nói chuyện với bên kia qua
 * generic API serial_write(). Nó không biết serial_t đó là UART, USB hay RAM.
 */
struct text_logger {
	const struct logger_ops *logger; /* vptr của hierarchy logger */
	serial_t out;                    /* cây cầu: handle tới hierarchy serial */
	enum log_level max_level;        /* lọc mức log: tính năng riêng phía abstraction */
	uint32_t seq;                    /* số thứ tự bản tin, nhảy cóc nghĩa là có bản bị mất */
	uint32_t dropped;
};

int text_logger_init(struct text_logger *self, serial_t out, enum log_level max_level);
logger_t text_logger_to_logger(struct text_logger *self);
uint32_t text_logger_dropped(const struct text_logger *self);

#endif /* TEXT_LOGGER_H */
