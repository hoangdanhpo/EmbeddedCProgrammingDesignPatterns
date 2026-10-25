#include "mem_serial.h"
#include "container_of.h"
#include "serial_ops.h"

#include <errno.h>
#include <string.h>

static int mem_serial_write(serial_t serial, const uint8_t *data, size_t len)
{
	struct mem_serial *self = CONTAINER_OF(serial, struct mem_serial, serial);

	/* Buffer đầy thì nhận một phần: đúng hợp đồng "có thể ít hơn len" */
	return (int)fifo_put(&self->tx, data, len);
}

static int mem_serial_read(serial_t serial, uint8_t *data, size_t len)
{
	struct mem_serial *self = CONTAINER_OF(serial, struct mem_serial, serial);

	return (int)fifo_get(&self->rx, data, len);
}

static const struct serial_ops mem_serial_ops = {
	.write = mem_serial_write,
	.read = mem_serial_read,
};

int mem_serial_init(struct mem_serial *self)
{
	if (!self) {
		return -EINVAL;
	}

	memset(self, 0, sizeof(*self));
	self->serial = &mem_serial_ops;
	fifo_init(&self->tx);
	fifo_init(&self->rx);
	return 0;
}

serial_t mem_serial_to_serial(struct mem_serial *self)
{
	return self ? &self->serial : NULL;
}

size_t mem_serial_take_tx(struct mem_serial *self, uint8_t *out, size_t cap)
{
	return fifo_get(&self->tx, out, cap);
}

void mem_serial_inject_rx(struct mem_serial *self, const uint8_t *data, size_t len)
{
	fifo_put(&self->rx, data, len);
}
