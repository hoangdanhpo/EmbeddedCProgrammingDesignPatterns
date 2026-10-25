#include "crc.h"
#include "spinlock.h"

#include <errno.h>
#include <stdbool.h>
#include <string.h>

/* CRC-32 (IEEE 802.3, dạng phản xạ như zlib): "123456789" cho ra 0xCBF43926 */
#define CRC32_POLY_REFLECTED 0xEDB88320u
#define CRC32_INIT           0xFFFFFFFFu
#define CRC32_XOR_OUT        0xFFFFFFFFu

struct crc {
	uint32_t dr; /* mô phỏng thanh ghi dữ liệu (DR) của khối CRC phần cứng */
};

/*
 * Trạng thái của singleton:
 *  - crc_instance: vùng nhớ của instance duy nhất (.bss)
 *  - crc_self:     NULL cho tới khi instance được khởi tạo (khởi tạo theo yêu cầu)
 *  - crc_owner:    ai đang giữ quyền dùng (NULL = rảnh)
 * Một lock bảo vệ cả ba, để "kiểm tra rồi gán" không bị thread khác chen vào.
 */
static struct crc crc_instance;
static struct crc *crc_self;
static struct crc *crc_owner;
static uint32_t crc_inits;
static struct spinlock crc_lock = SPINLOCK_INIT;

/* Mô phỏng phần cứng: mỗi byte ghi vào DR được trộn vào CRC */
static void crc_hw_write_byte(struct crc *self, uint8_t byte)
{
	self->dr ^= byte;
	for (int bit = 0; bit < 8; bit++) {
		self->dr = (self->dr & 1u) ? (self->dr >> 1) ^ CRC32_POLY_REFLECTED : self->dr >> 1;
	}
}

static int crc_init(struct crc *self)
{
	memset(self, 0, sizeof(*self));
	self->dr = CRC32_INIT;
	crc_inits++;
	return 0;
}

/*
 * Khởi tạo theo yêu cầu, an toàn với thread: phép kiểm tra crc_self và phép
 * gán nằm trong CÙNG một vùng khóa. Hàm gọi phải đang giữ crc_lock.
 */
static struct crc *crc_get_singleton_locked(void)
{
	if (!crc_self) {
		if (crc_init(&crc_instance) == 0) {
			crc_self = &crc_instance;
		}
	}
	return crc_self;
}

struct crc *crc_acquire(void)
{
	struct crc *self;

	spin_lock(&crc_lock);
	self = crc_get_singleton_locked();
	if (self && !crc_owner) {
		crc_owner = self;
	} else {
		self = NULL; /* đang có chủ khác: từ chối thay vì chờ, để caller tự quyết */
	}
	spin_unlock(&crc_lock);
	return self;
}

int crc_release(struct crc **self)
{
	int ret = -EPERM;

	if (!self || !*self) {
		return -EINVAL;
	}

	spin_lock(&crc_lock);
	if (*self == crc_owner) {
		crc_owner = NULL;
		*self = NULL; /* xóa con trỏ của caller: dùng tiếp sẽ bị bắt ngay bằng -EINVAL */
		ret = 0;
	}
	spin_unlock(&crc_lock);
	return ret;
}

int crc_reset(struct crc *self)
{
	if (!self) {
		return -EINVAL;
	}

	self->dr = CRC32_INIT;
	return 0;
}

int crc_feed(struct crc *self, const uint8_t *data, size_t len)
{
	if (!self || (!data && len > 0)) {
		return -EINVAL;
	}

	for (size_t i = 0; i < len; i++) {
		crc_hw_write_byte(self, data[i]);
	}
	return 0;
}

int crc_result(const struct crc *self, uint32_t *out)
{
	if (!self || !out) {
		return -EINVAL;
	}

	*out = self->dr ^ CRC32_XOR_OUT;
	return 0;
}

int crc_compute(const uint8_t *data, size_t len, uint32_t *out)
{
	struct crc *self = crc_acquire();
	int ret;

	if (!self) {
		return -EBUSY;
	}

	ret = crc_reset(self);
	if (ret == 0) {
		ret = crc_feed(self, data, len);
	}
	if (ret == 0) {
		ret = crc_result(self, out);
	}
	crc_release(&self); /* luôn trả lại, kể cả khi lỗi, nếu không khối CRC bị khóa vĩnh viễn */
	return ret;
}

uint32_t crc_init_count(void)
{
	uint32_t count;

	spin_lock(&crc_lock);
	count = crc_inits;
	spin_unlock(&crc_lock);
	return count;
}
