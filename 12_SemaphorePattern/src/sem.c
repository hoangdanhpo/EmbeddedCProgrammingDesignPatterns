#include "sem.h"

#include "arch_irq.h"

#include <errno.h>

/*
 * Một spinlock dùng chung cho MỌI semaphore, như sem.c của Zephyr. Vùng găng
 * chỉ vài lệnh, nên tách lock theo từng instance gần như không lợi gì mà tốn RAM
 * (sách, Spinlock Pattern: "often little sense in multiple instances").
 */
static struct spinlock sem_lock = SPINLOCK_INIT;

int sem_init(struct sem *self, uint32_t initial_count, uint32_t limit)
{
	if (!self || limit == 0 || initial_count > limit) {
		return -EINVAL;
	}

	self->count = initial_count;
	self->limit = limit;
	wait_q_init(&self->wait_q);
	return 0;
}

int sem_give(struct sem *self)
{
	if (!self) {
		return -EINVAL;
	}

	struct spinlock_key key = spinlock_lock(&sem_lock);
	struct kthread *thread = wait_q_unpend_first(&self->wait_q);

	if (thread) {
		/* Trao thẳng "tài nguyên" cho thread đang chờ: count không đổi, vì
		 * nếu tăng lên thì thread vừa thức lại phải giảm xuống ngay */
		kthread_ready(thread, 0);
	} else if (self->count != self->limit) {
		self->count++;
	}

	/* Zephyr gọi z_reschedule(&lock, key) ở đây: thả lock và, nếu thread vừa
	 * thức có ưu tiên cao hơn, chuyển CPU sang nó ngay. Trên PC, Windows lo
	 * việc cho thread đó chạy. */
	spinlock_unlock(&sem_lock, key);
	return 0;
}

int sem_take(struct sem *self, int32_t timeout_ms)
{
	if (!self) {
		return -EINVAL;
	}
	if (arch_is_in_isr() && timeout_ms != K_NO_WAIT) {
		/* ISR không phải thread, không có TCB để xếp vào hàng đợi.
		 * Zephyr dừng hẳn bằng __ASSERT; ở đây trả lỗi cho dễ quan sát. */
		return -EINVAL;
	}

	struct spinlock_key key = spinlock_lock(&sem_lock);

	if (self->count > 0) {
		/* Đường nhanh: còn tài nguyên rảnh, không cần đụng tới bộ lập lịch */
		self->count--;
		spinlock_unlock(&sem_lock, key);
		return 0;
	}

	if (timeout_ms == K_NO_WAIT) {
		spinlock_unlock(&sem_lock, key);
		return -EBUSY;
	}

	/* Thả spinlock và ngủ trong cùng một bước: hàm này return khi đã thả lock */
	return wait_q_pend_curr(&self->wait_q, &sem_lock, key, timeout_ms);
}

uint32_t sem_count_get(struct sem *self)
{
	if (!self) {
		return 0;
	}

	struct spinlock_key key = spinlock_lock(&sem_lock);
	uint32_t count = self->count;
	spinlock_unlock(&sem_lock, key);
	return count;
}
