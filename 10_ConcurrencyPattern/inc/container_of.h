#ifndef CONTAINER_OF_H
#define CONTAINER_OF_H

#include <stddef.h>

/*
 * Từ con trỏ tới một member, lấy lại con trỏ tới struct bao ngoài nó.
 * Nhờ vậy work item chỉ cần là một member, không cần con trỏ "user data".
 */
#define CONTAINER_OF(ptr, type, member) ((type *)((char *)(ptr) - offsetof(type, member)))

#endif /* CONTAINER_OF_H */
