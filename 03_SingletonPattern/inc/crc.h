#ifndef CRC_H
#define CRC_H

#include <stddef.h>
#include <stdint.h>

/*
 * Khối CRC phần cứng: chip chỉ có MỘT khối CRC, nên đây là singleton thật sự.
 *
 * API của khối CRC là STATEFUL (reset -> feed -> result phải đúng thứ tự),
 * nên ngoài "chỉ một instance" còn phải bảo đảm "chỉ một NGƯỜI DÙNG tại một
 * thời điểm": ai muốn dùng phải acquire, dùng xong phải release.
 *
 * struct crc là opaque. Caller cầm con trỏ trong lúc sở hữu; con trỏ đó chính
 * là "giấy chứng nhận" mình đang là chủ.
 */
struct crc;

/* Trả NULL nếu đang có người khác giữ */
struct crc *crc_acquire(void);

/* Đặt con trỏ của caller về NULL; -EPERM nếu caller không phải chủ */
int crc_release(struct crc **self);

int crc_reset(struct crc *self);
int crc_feed(struct crc *self, const uint8_t *data, size_t len);
int crc_result(const struct crc *self, uint32_t *out);

/*
 * Bản stateless: tự acquire/reset/feed/result/release trong một lần gọi.
 * Trả -EBUSY nếu khối CRC đang bận.
 */
int crc_compute(const uint8_t *data, size_t len, uint32_t *out);

/* Chỉ để demo: số lần instance thật sự được khởi tạo (phải luôn là 0 hoặc 1) */
uint32_t crc_init_count(void);

#endif /* CRC_H */
