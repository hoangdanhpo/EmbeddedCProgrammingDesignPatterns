#ifndef POWER_TRAIT_H
#define POWER_TRAIT_H

#include <errno.h>
#include <stddef.h>

/* Trait "quản lý nguồn được": thiết bị nào tắt/bật nguồn được thì có trait này */
struct power_trait {
	int (*suspend)(struct power_trait *self);
	int (*resume)(struct power_trait *self);
};

static inline int power_trait_suspend(struct power_trait *self)
{
	if (!self) {
		return -EINVAL;
	}
	return self->suspend ? self->suspend(self) : -ENOSYS;
}

static inline int power_trait_resume(struct power_trait *self)
{
	if (!self) {
		return -EINVAL;
	}
	return self->resume ? self->resume(self) : -ENOSYS;
}

#endif /* POWER_TRAIT_H */
