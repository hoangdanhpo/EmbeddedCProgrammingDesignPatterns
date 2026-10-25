#ifndef LOGGER_H
#define LOGGER_H

#include <stdint.h>

/*
 * Logger hệ thống: một SINGLETON khởi tạo tự động lúc khởi động (SYS_INIT).
 *
 * Không có struct logger, không có self: caller không cầm instance nào cả.
 * Mọi hàm đều "stateless" theo nghĩa của sách: gọi theo thứ tự nào, từ thread
 * nào cũng cho kết quả đúng, vì mỗi hàm tự khóa và mở khóa bên trong.
 */

enum logger_level {
	LOGGER_LEVEL_DEBUG,
	LOGGER_LEVEL_INFO,
	LOGGER_LEVEL_WARN,
	LOGGER_LEVEL_ERROR,
};

/*
 * Sink: nơi nhận dòng log đã định dạng (console, UART, flash...).
 * Sink là object do CALLER cấp phát và sở hữu, logger chỉ giữ con trỏ.
 * Đây là vai trò "hub đăng ký" điển hình của singleton.
 */
struct logger_sink {
	const char *name;
	void (*write)(struct logger_sink *self, const char *line);
	struct logger_sink *next; /* logger dùng để xâu chuỗi; caller không đụng vào */
};

struct logger_stats {
	uint32_t written;  /* số dòng đã gửi tới sink */
	uint32_t filtered; /* số dòng bị bỏ vì thấp hơn level */
};

int logger_add_sink(struct logger_sink *sink);
int logger_set_level(enum logger_level level);
int logger_write(enum logger_level level, const char *tag, const char *msg);

/* Trả một bản chụp (copy) chứ không trả con trỏ vào state bên trong */
int logger_get_stats(struct logger_stats *out);

#endif /* LOGGER_H */
