#include "usb_cdc.h"
#include "container_of.h"
#include "serial_ops.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

static int usb_cdc_serial_write(serial_t serial, const uint8_t *data, size_t len)
{
	struct usb_cdc *self = CONTAINER_OF(serial, struct usb_cdc, serial);

	/* Hợp đồng của serial_write: đường truyền chưa sẵn sàng thì trả -ENOTCONN */
	if (!self->connected) {
		return -ENOTCONN;
	}

	for (size_t off = 0; off < len; off += USB_CDC_MAX_PACKET) {
		size_t n = len - off < USB_CDC_MAX_PACKET ? len - off : USB_CDC_MAX_PACKET;

		printf("[USB ep 0x%02X] packet %lu: ", (unsigned)self->ep_in,
		       (unsigned long)self->packets_sent);
		for (size_t i = 0; i < n; i++) {
			printf("%02X ", (unsigned)data[off + i]);
		}
		printf("\n");
		self->packets_sent++;
	}
	return (int)len;
}

static int usb_cdc_serial_read(serial_t serial, uint8_t *data, size_t len)
{
	struct usb_cdc *self = CONTAINER_OF(serial, struct usb_cdc, serial);

	if (!self->connected) {
		return -ENOTCONN;
	}
	return (int)fifo_get(&self->rx, data, len);
}

static const struct serial_ops usb_cdc_serial_ops = {
	.write = usb_cdc_serial_write,
	.read = usb_cdc_serial_read,
};

int usb_cdc_init(struct usb_cdc *self, uint8_t ep_in)
{
	if (!self) {
		return -EINVAL;
	}

	memset(self, 0, sizeof(*self));
	self->serial = &usb_cdc_serial_ops;
	self->ep_in = ep_in;
	fifo_init(&self->rx);
	return 0;
}

serial_t usb_cdc_to_serial(struct usb_cdc *self)
{
	return self ? &self->serial : NULL;
}

void usb_cdc_set_connected(struct usb_cdc *self, bool connected)
{
	self->connected = connected;
}

void usb_cdc_host_send(struct usb_cdc *self, const uint8_t *data, size_t len)
{
	fifo_put(&self->rx, data, len);
}
