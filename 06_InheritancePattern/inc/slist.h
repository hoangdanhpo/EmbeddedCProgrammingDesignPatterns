#ifndef SLIST_H
#define SLIST_H

#include <stdbool.h>
#include <stddef.h>

/*
 * "Base class" dùng chung cho mọi thứ cần xếp thành danh sách, mô phỏng
 * sys_slist_t của Zephyr. Struct nào muốn vào danh sách thì "kế thừa" node
 * bằng cách CHỨA nó (struct led trong bài này). Mọi thuật toán danh sách
 * viết một lần ở đây và dùng lại cho mọi kiểu dữ liệu.
 */
struct slist_node {
	struct slist_node *next;
};

struct slist {
	struct slist_node *head;
	struct slist_node *tail;
};

void slist_init(struct slist *self);
bool slist_is_empty(const struct slist *self);
void slist_append(struct slist *self, struct slist_node *node);
bool slist_contains(const struct slist *self, const struct slist_node *node);
bool slist_find_and_remove(struct slist *self, struct slist_node *node);

/*
 * Duyệt "an toàn": đọc sẵn node->next vào tmp TRƯỚC khi chạy thân vòng lặp,
 * nên thân vòng lặp được phép gỡ chính node đang duyệt (ví dụ callback tự
 * hủy đăng ký) mà không làm đứt vòng lặp.
 */
#define SLIST_FOR_EACH_NODE_SAFE(list, node, tmp)                         \
	for ((node) = (list)->head, (tmp) = (node) ? (node)->next : NULL; \
	     (node) != NULL;                                              \
	     (node) = (tmp), (tmp) = (node) ? (node)->next : NULL)

#endif /* SLIST_H */
