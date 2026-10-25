#ifndef LOGGER_H
#define LOGGER_H

/*
 * Đầu cầu phía ứng dụng (abstraction): generic API của hierarchy "logger".
 * Ứng dụng chỉ include file này. Nó không biết log được định dạng thế nào
 * (text hay frame nhị phân) và cũng không biết log đi ra đâu (UART, USB, RAM):
 * file này thậm chí không include serial.h.
 */
struct logger_ops;

typedef const struct logger_ops **logger_t;

enum log_level {
	LOG_ERR = 0,
	LOG_WRN,
	LOG_INF,
	LOG_DBG,
};

/*
 * Hợp đồng: ghi một bản tin log, không chặn.
 * Trả về 0 khi đã giao bản tin cho đường truyền (hoặc khi logger chủ động bỏ
 * qua vì mức log bị lọc), mã lỗi âm khi không giao được: -EINVAL (tham số sai),
 * -EMSGSIZE (bản tin quá dài), -EIO (đường truyền nhận thiếu), hoặc mã lỗi mà
 * đường truyền trả về (ví dụ -ENOTCONN).
 */
int logger_log(logger_t logger, enum log_level level, const char *msg);

#endif /* LOGGER_H */
