#ifndef SPINLOCK_H
#define SPINLOCK_H

/*
 * Spinlock tối giản bằng C11 atomic_flag, đứng thay cho k_spinlock của Zephyr.
 *
 * Header private (nằm trong src/): chỉ implementation của singleton cần lock,
 * caller không bao giờ thấy nó.
 *
 * Khác biệt với k_spin_lock thật: k_spin_lock còn tắt interrupt trên CPU hiện
 * tại và trả về "key" để bật lại. Bản PC này chỉ chặn giữa các thread. Bài 11
 * (Spinlock) sẽ đi sâu vào chuyện này.
 */
#include <stdatomic.h>

struct spinlock {
	atomic_flag flag;
};

/* Khởi tạo tĩnh được, nên lock có sẵn ngay từ trước main() */
#define SPINLOCK_INIT { ATOMIC_FLAG_INIT }

static inline void spin_lock(struct spinlock *lock)
{
	/* test_and_set trả giá trị CŨ: còn true nghĩa là người khác đang giữ, quay lại thử */
	while (atomic_flag_test_and_set_explicit(&lock->flag, memory_order_acquire)) {
	}
}

static inline void spin_unlock(struct spinlock *lock)
{
	atomic_flag_clear_explicit(&lock->flag, memory_order_release);
}

#endif /* SPINLOCK_H */
