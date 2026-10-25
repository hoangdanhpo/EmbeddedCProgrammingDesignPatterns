#ifndef BUTTON_H
#define BUTTON_H

#include "slist.h"

#include <stdbool.h>

enum button_event {
	BUTTON_EVENT_PRESSED,
	BUTTON_EVENT_RELEASED,
};

/*
 * Cấu trúc DUY NHẤT mà button và observer cùng biết. Observer nhúng struct
 * này vào object của mình, nên từ con trỏ cb, handler dùng CONTAINER_OF để
 * lấy lại object observer. Button không biết (và không cần biết) observer
 * là kiểu gì.
 */
struct button_callback {
	struct slist_node node; /* Do button quản lý, observer không đụng vào */
	void (*handler)(struct button_callback *cb, enum button_event event);
};

/*
 * Object Pattern: caller cấp phát. Không có con trỏ tới observer nào cụ thể,
 * chỉ có một danh sách các button_callback.
 */
struct button {
	const char *name;
	bool pressed;
	struct slist callbacks;
};

int button_init(struct button *self, const char *name);
int button_deinit(struct button *self);
int button_add_callback(struct button *self, struct button_callback *cb);
int button_remove_callback(struct button *self, struct button_callback *cb);

/*
 * Gọi từ ISR của chân GPIO (trong demo: gọi từ main để mô phỏng). Mọi
 * handler chạy NGAY TRONG ngữ cảnh của hàm gọi này, nên phải ngắn và không block.
 */
int button_set_level(struct button *self, bool pressed);

bool button_is_pressed(const struct button *self);

#endif /* BUTTON_H */
