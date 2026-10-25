#ifndef SPINLOCK_H
#define SPINLOCK_H

#include "arch_irq.h"

#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

/*
 * Spinlock = che ngắt trên CPU hiện tại + (nếu nhiều core) giành một biến atomic.
 *
 * SPINLOCK_SMP = 0: build cho MCU đơn nhân. Phần atomic bị bỏ hẳn, spinlock chỉ
 *                   còn là arch_irq_lock()/arch_irq_unlock().
 * SPINLOCK_VALIDATE = 1: ghi lại CPU đang giữ lock để bắt lỗi lock đệ quy và
 *                   unlock từ CPU khác (giống CONFIG_SPIN_VALIDATE của Zephyr).
 */
#ifndef SPINLOCK_SMP
#define SPINLOCK_SMP 1
#endif

#ifndef SPINLOCK_VALIDATE
#define SPINLOCK_VALIDATE 1
#endif

/*
 * Struct để public (Object Pattern, không Opaque) vì hai lý do:
 * spinlock phải nhúng được thẳng vào object khác (struct rx_ring { ...; struct
 * spinlock lock; }) và các hàm phải inline được ở mọi nơi gọi.
 */
struct spinlock {
#if SPINLOCK_SMP
	atomic_int locked; /* 0 = tự do, 1 = có CPU đang giữ */
#endif
#if SPINLOCK_VALIDATE
	atomic_uint owner; /* (id CPU + 1) đang giữ lock, 0 = không ai */
#endif
#if !SPINLOCK_SMP && !SPINLOCK_VALIDATE
	char unused; /* C chuẩn không cho struct rỗng */
#endif
};

/*
 * Bọc key trong struct để compiler bắt lỗi truyền nhầm một số unsigned bất kỳ
 * vào spinlock_unlock(). Key nằm trên stack của người lock, nên lock lồng nhau
 * mỗi tầng có key riêng.
 */
struct spinlock_key {
	unsigned int key;
};

#define SPINLOCK_INIT {0}

/* Gợi ý cho CPU biết ta đang quay chờ: giảm điện và nhường tài nguyên cho
 * hyper-thread anh em. Cortex-M không có lệnh tương đương bắt buộc. */
static inline void spinlock_cpu_relax(void)
{
#if defined(__x86_64__) || defined(__i386__)
	__builtin_ia32_pause();
#endif
}

static inline void spinlock_validate_fail(const char *what)
{
	fprintf(stderr, "spinlock: %s (cpu %u)\n", what, arch_curr_cpu_id());
	abort();
}

static inline struct spinlock_key spinlock_lock(struct spinlock *self)
{
	struct spinlock_key k;

	(void)self; /* khi SPINLOCK_SMP = 0 và SPINLOCK_VALIDATE = 0 thì không dùng tới */

	/* Bước 1: che ngắt TRƯỚC. Nếu che sau khi đã giành được lock, một ISR trên
	 * chính CPU này có thể chen vào giữa và quay chờ mãi lock mà ta đang giữ. */
	k.key = arch_irq_lock();

#if SPINLOCK_VALIDATE
	if (atomic_load_explicit(&self->owner, memory_order_relaxed) == arch_curr_cpu_id() + 1) {
		spinlock_validate_fail("recursive lock: this CPU already holds it");
	}
#endif

#if SPINLOCK_SMP
	/* Bước 2: giành biến atomic bằng compare-and-swap: "nếu đang là 0 thì đặt 1".
	 * Hai CPU cùng thử thì phần cứng bảo đảm chỉ một bên thành công. */
	int expected = 0;
	while (!atomic_compare_exchange_weak_explicit(&self->locked, &expected, 1,
						      memory_order_acquire,
						      memory_order_relaxed)) {
		/* CAS thất bại ghi giá trị hiện tại vào expected, phải đặt lại 0.
		 * Chỉ đọc (không ghi) trong lúc chờ để không giành cache line liên tục. */
		do {
			spinlock_cpu_relax();
		} while (atomic_load_explicit(&self->locked, memory_order_relaxed) != 0);
		expected = 0;
	}
#endif

#if SPINLOCK_VALIDATE
	atomic_store_explicit(&self->owner, arch_curr_cpu_id() + 1, memory_order_relaxed);
#endif
	return k;
}

static inline void spinlock_unlock(struct spinlock *self, struct spinlock_key key)
{
	(void)self;

#if SPINLOCK_VALIDATE
	if (atomic_load_explicit(&self->owner, memory_order_relaxed) != arch_curr_cpu_id() + 1) {
		spinlock_validate_fail("unlock by a CPU that does not hold the lock");
	}
	atomic_store_explicit(&self->owner, 0, memory_order_relaxed);
#endif

#if SPINLOCK_SMP
	/* release: mọi lệnh ghi trong vùng găng phải hiện ra với CPU khác
	 * TRƯỚC khi chúng thấy lock được thả */
	atomic_store_explicit(&self->locked, 0, memory_order_release);
#endif

	/* Khôi phục đúng trạng thái ngắt trước khi lock, không mở bừa */
	arch_irq_unlock(key.key);
}

#endif /* SPINLOCK_H */
