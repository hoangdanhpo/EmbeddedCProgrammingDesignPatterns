#include "work_queue.h"

#include <errno.h>
#include <stddef.h>

int work_init(struct work *self, void (*handler)(struct work *work))
{
	if (!self || !handler) {
		return -EINVAL;
	}

	self->handler = handler;
	self->next = NULL;
	self->pending = false;
	return 0;
}

static void *work_queue_thread(void *arg)
{
	struct work_queue *self = arg;

	for (;;) {
		pthread_mutex_lock(&self->lock);
		while (!self->head && self->running) {
			/* Không có việc: thread ngủ hẳn, không tốn CPU, tới khi submit đánh thức */
			pthread_cond_wait(&self->cond, &self->lock);
		}
		if (!self->head) {
			/* running == false và hàng đợi đã cạn */
			pthread_mutex_unlock(&self->lock);
			return NULL;
		}

		struct work *work = self->head;

		self->head = work->next;
		if (!self->head) {
			self->tail = NULL;
		}
		work->next = NULL;
		work->pending = false;
		pthread_mutex_unlock(&self->lock);

		/* Chạy handler NGOÀI lock: handler chậm không chặn các thread đang submit */
		work->handler(work);
		self->executed++; /* chỉ thread worker ghi biến này */
	}
}

int work_queue_start(struct work_queue *self)
{
	if (!self) {
		return -EINVAL;
	}

	self->head = NULL;
	self->tail = NULL;
	self->running = true;
	self->executed = 0;
	if (pthread_mutex_init(&self->lock, NULL) != 0) {
		return -EAGAIN;
	}
	if (pthread_cond_init(&self->cond, NULL) != 0) {
		pthread_mutex_destroy(&self->lock);
		return -EAGAIN;
	}
	/* pthread trả mã lỗi DƯƠNG (kiểu POSIX): dịch sang quy ước âm của bài 09 */
	if (pthread_create(&self->thread, NULL, work_queue_thread, self) != 0) {
		pthread_cond_destroy(&self->cond);
		pthread_mutex_destroy(&self->lock);
		return -EAGAIN;
	}
	return 0;
}

int work_queue_submit(struct work_queue *self, struct work *work)
{
	int ret = 0;

	if (!self || !work) {
		return -EINVAL;
	}

	pthread_mutex_lock(&self->lock);
	if (!self->running) {
		ret = -EINVAL;
	} else if (work->pending) {
		ret = 1;
	} else {
		work->pending = true;
		work->next = NULL;
		if (self->tail) {
			self->tail->next = work;
		} else {
			self->head = work;
		}
		self->tail = work;
		pthread_cond_signal(&self->cond);
	}
	pthread_mutex_unlock(&self->lock);
	return ret;
}

int work_queue_stop(struct work_queue *self)
{
	if (!self) {
		return -EINVAL;
	}

	pthread_mutex_lock(&self->lock);
	self->running = false;
	pthread_cond_signal(&self->cond);
	pthread_mutex_unlock(&self->lock);

	pthread_join(self->thread, NULL);
	pthread_cond_destroy(&self->cond);
	pthread_mutex_destroy(&self->lock);
	return 0;
}
