#include "button.h"
#include "container_of.h"

#include <errno.h>
#include <string.h>

int button_init(struct button *self, const char *name)
{
	if (!self || !name) {
		return -EINVAL;
	}

	memset(self, 0, sizeof(*self));
	self->name = name;
	slist_init(&self->callbacks);
	return 0;
}

int button_deinit(struct button *self)
{
	if (!self) {
		return -EINVAL;
	}

	/*
	 * Còn observer đăng ký mà button biến mất thì observer sẽ giữ con trỏ
	 * treo tới button. Từ chối để lộ sai thứ tự hủy ngay lúc chạy thử.
	 */
	if (!slist_is_empty(&self->callbacks)) {
		return -EBUSY;
	}

	memset(self, 0, sizeof(*self));
	return 0;
}

int button_add_callback(struct button *self, struct button_callback *cb)
{
	if (!self || !cb || !cb->handler) {
		return -EINVAL;
	}

	/* Thêm một node hai lần sẽ tạo vòng lặp vô hạn trong danh sách */
	if (slist_contains(&self->callbacks, &cb->node)) {
		return -EALREADY;
	}

	slist_append(&self->callbacks, &cb->node);
	return 0;
}

int button_remove_callback(struct button *self, struct button_callback *cb)
{
	if (!self || !cb) {
		return -EINVAL;
	}

	return slist_find_and_remove(&self->callbacks, &cb->node) ? 0 : -ENOENT;
}

static void button_notify(struct button *self, enum button_event event)
{
	struct slist_node *node;
	struct slist_node *next;

	/* Bản SAFE: handler được phép tự gỡ mình khỏi danh sách giữa chừng */
	SLIST_FOR_EACH_NODE_SAFE(&self->callbacks, node, next) {
		struct button_callback *cb = CONTAINER_OF(node, struct button_callback, node);

		cb->handler(cb, event);
	}
}

int button_set_level(struct button *self, bool pressed)
{
	if (!self) {
		return -EINVAL;
	}

	/* Chỉ báo khi trạng thái thật sự đổi: cùng mức thì không phải sự kiện */
	if (self->pressed == pressed) {
		return 0;
	}

	self->pressed = pressed;
	button_notify(self, pressed ? BUTTON_EVENT_PRESSED : BUTTON_EVENT_RELEASED);
	return 0;
}

bool button_is_pressed(const struct button *self)
{
	return self ? self->pressed : false;
}
