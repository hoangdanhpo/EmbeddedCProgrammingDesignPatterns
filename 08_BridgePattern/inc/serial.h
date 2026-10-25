#ifndef SERIAL_H
#define SERIAL_H

#include <stddef.h>
#include <stdint.h>

/*
 * Generic API (public): thứ duy nhất code ứng dụng cần include để dùng
 * một cổng serial bất kỳ. Struct serial_ops chỉ được KHAI BÁO ở đây; định
 * nghĩa nằm trong src/serial_ops.h, chỉ generic API và các implementation
 * mới thấy. Nhờ vậy ứng dụng không thể tự gọi (*handle)->write(...) hay
 * sửa bảng hàm: mọi lời gọi bắt buộc đi qua các hàm bên dưới.
 */
struct serial_ops;

/*
 * Handle của interface: con trỏ tới field "const struct serial_ops *" nằm
 * bên trong object implementation (uart, usb_cdc, ...). Dùng con trỏ cấp
 * hai để implementation lấy lại được object của nó bằng CONTAINER_OF.
 */
typedef const struct serial_ops **serial_t;

/*
 * Hợp đồng chung mà MỌI implementation phải tuân theo:
 *
 * serial_write: nhận tối đa len byte để gửi đi, không chặn (non-blocking).
 *   Trả về số byte đã nhận (0..len; có thể ít hơn len khi buffer đầy),
 *   hoặc mã lỗi âm: -EINVAL (tham số sai), -ENOTCONN (đường truyền chưa sẵn
 *   sàng), -ENOSYS (implementation không hỗ trợ thao tác này).
 *
 * serial_read: chép tối đa len byte đã nhận được vào data, không chặn.
 *   Trả về số byte đã chép (0 = chưa có gì), hoặc mã lỗi âm như trên.
 */
int serial_write(serial_t serial, const uint8_t *data, size_t len);
int serial_read(serial_t serial, uint8_t *data, size_t len);

#endif /* SERIAL_H */
