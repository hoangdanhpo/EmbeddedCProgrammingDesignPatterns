#include "slist.h"

void slist_init(struct slist *self)
{
	self->head = NULL;
	self->tail = NULL;
}

bool slist_is_empty(const struct slist *self)
{
	return self->head == NULL;
}

void slist_append(struct slist *self, struct slist_node *node)
{
	/* Node có thể còn rác từ lần dùng trước: luôn đặt lại next */
	node->next = NULL;

	if (self->tail) {
		self->tail->next = node;
	} else {
		self->head = node;
	}
	self->tail = node;
}

bool slist_contains(const struct slist *self, const struct slist_node *node)
{
	for (const struct slist_node *it = self->head; it; it = it->next) {
		if (it == node) {
			return true;
		}
	}
	return false;
}

bool slist_find_and_remove(struct slist *self, struct slist_node *node)
{
	struct slist_node *prev = NULL;

	for (struct slist_node *it = self->head; it; prev = it, it = it->next) {
		if (it != node) {
			continue;
		}

		if (prev) {
			prev->next = it->next;
		} else {
			self->head = it->next;
		}
		if (self->tail == it) {
			self->tail = prev;
		}
		it->next = NULL;
		return true;
	}
	return false;
}
