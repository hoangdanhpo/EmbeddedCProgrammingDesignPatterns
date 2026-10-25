#ifndef CONDVAR_H
#define CONDVAR_H

#include "mutex.h"

#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>

/* Một thread đang chờ trên condvar. Node nằm trên stack của chính thread đó. */
struct condvar_waiter {
	pthread_cond_t wake;
	bool woken;                  /* Đã được signal/broadcast lấy ra khỏi hàng đợi */
	struct condvar_waiter *next;
};

/*
 * Condition variable: hàng đợi các thread chờ "có gì đó đã thay đổi".
 * Nó KHÔNG chứa điều kiện; điều kiện (predicate) là dữ liệu của caller,
 * được bảo vệ bởi một struct mutex mà caller truyền vào condvar_wait().
 *
 * guard đóng vai spinlock của k_condvar: bảo vệ hàng đợi, và giúp
 * "nhả mutex + vào hàng đợi" thành một bước không ai chen vào được.
 */
struct condvar {
	pthread_mutex_t guard;
	struct condvar_waiter *head;
	struct condvar_waiter *tail;
};

int condvar_init(struct condvar *self);
int condvar_deinit(struct condvar *self);

/*
 * Gọi khi ĐANG GIỮ mutex (đúng một tầng). Nhả mutex và ngủ như một bước nguyên tử;
 * khi thức dậy (signal, broadcast hoặc hết giờ) thì lấy lại mutex rồi mới return.
 * timeout_ms: MUTEX_FOREVER hoặc số mili giây >= 0.
 * Trả về 0 khi được đánh thức; -EAGAIN khi hết giờ (vẫn đang giữ mutex);
 * -EPERM nếu không giữ mutex; -EDEADLK nếu mutex đang bị khóa lồng; -EINVAL nếu tham số sai.
 * Return 0 KHÔNG có nghĩa là điều kiện đã đúng: luôn kiểm tra lại trong vòng while.
 */
int condvar_wait(struct condvar *self, struct mutex *mutex, int32_t timeout_ms);

/* Đánh thức một thread đang chờ (nếu có). Không cần giữ mutex. Trả về 0 hoặc -EINVAL. */
int condvar_signal(struct condvar *self);

/* Đánh thức mọi thread đang chờ. Trả về số thread được đánh thức, hoặc -EINVAL. */
int condvar_broadcast(struct condvar *self);

#endif /* CONDVAR_H */
