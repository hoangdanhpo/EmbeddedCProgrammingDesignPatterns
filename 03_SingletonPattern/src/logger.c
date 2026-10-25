#include "logger.h"
#include "spinlock.h"
#include "sys_init.h"

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define LOGGER_LINE_LEN 96

/*
 * Bên trong singleton vẫn là Object Pattern: toàn bộ state gom vào một struct,
 * và các hàm private nhận self. Chỉ có đúng MỘT instance, nằm ở dưới.
 */
struct logger {
	bool ready;
	enum logger_level min_level;
	uint32_t seq;
	struct logger_stats stats;
	struct logger_sink *sinks;
	char line[LOGGER_LINE_LEN]; /* buffer định dạng, dùng chung nên phải giữ lock */
};

/*
 * Instance duy nhất: static nên không file nào khác nhìn thấy tên của nó.
 * Lock tách riêng và khởi tạo tĩnh, để nó dùng được ngay cả trước khi
 * logger_init() chạy.
 */
static struct logger logger_instance;
static struct spinlock logger_lock = SPINLOCK_INIT;

static const char *const logger_level_names[] = { "DBG", "INF", "WRN", "ERR" };

/* ---- Phần private: Object Pattern bình thường, nhận self ---- */

static int logger_init(struct logger *self)
{
	memset(self, 0, sizeof(*self));
	self->min_level = LOGGER_LEVEL_INFO;
	self->ready = true;
	return 0;
}

static int logger_do_add_sink(struct logger *self, struct logger_sink *sink)
{
	struct logger_sink **pos = &self->sinks;

	while (*pos) {
		if (*pos == sink) {
			return -EALREADY; /* đăng ký hai lần sẽ tạo vòng lặp vô tận trong danh sách */
		}
		pos = &(*pos)->next;
	}
	sink->next = NULL;
	*pos = sink; /* nối vào cuối để sink nhận log theo đúng thứ tự đăng ký */
	return 0;
}

static int logger_do_write(struct logger *self, enum logger_level level, const char *tag,
			   const char *msg)
{
	if (level < self->min_level) {
		self->stats.filtered++;
		return 0;
	}

	self->seq++;
	snprintf(self->line, sizeof(self->line), "[%04lu] %s %s: %s", (unsigned long)self->seq,
		 logger_level_names[level], tag, msg);

	/* Sink được gọi khi đang giữ lock: sink không được gọi ngược lại logger_write */
	for (struct logger_sink *s = self->sinks; s; s = s->next) {
		s->write(s, self->line);
	}
	self->stats.written++;
	return 0;
}

/* ---- Khởi tạo tự động trước main(), giống SYS_INIT của Zephyr ---- */

static int logger_sys_init(void)
{
	return logger_init(&logger_instance);
}

SYS_INIT(logger_sys_init, SYS_INIT_PRIO_SUBSYSTEM)

/* ---- Phần public: không có self, mỗi hàm tự khóa và mở khóa ---- */

int logger_add_sink(struct logger_sink *sink)
{
	int ret;

	if (!sink || !sink->write) {
		return -EINVAL;
	}

	spin_lock(&logger_lock);
	ret = logger_instance.ready ? logger_do_add_sink(&logger_instance, sink) : -ENODEV;
	spin_unlock(&logger_lock);
	return ret;
}

int logger_set_level(enum logger_level level)
{
	/* Ép sang unsigned: một phép so sánh bắt được cả giá trị âm lẫn quá lớn */
	if ((unsigned)level > (unsigned)LOGGER_LEVEL_ERROR) {
		return -EINVAL;
	}

	spin_lock(&logger_lock);
	logger_instance.min_level = level;
	spin_unlock(&logger_lock);
	return 0;
}

int logger_write(enum logger_level level, const char *tag, const char *msg)
{
	int ret;

	if ((unsigned)level > (unsigned)LOGGER_LEVEL_ERROR || !tag || !msg) {
		return -EINVAL;
	}

	spin_lock(&logger_lock);
	ret = logger_instance.ready ? logger_do_write(&logger_instance, level, tag, msg) : -ENODEV;
	spin_unlock(&logger_lock);
	return ret;
}

int logger_get_stats(struct logger_stats *out)
{
	if (!out) {
		return -EINVAL;
	}

	spin_lock(&logger_lock);
	*out = logger_instance.stats;
	spin_unlock(&logger_lock);
	return 0;
}
