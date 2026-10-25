#ifndef USB_CDC_H
#define USB_CDC_H

#include "fifo.h"
#include "serial.h"

#include <stdbool.h>
#include <stdint.h>

/* Gói tối đa của endpoint bulk. USB Full Speed là 64; demo để 8 cho dễ thấy */
#define USB_CDC_MAX_PACKET 8

/*
 * Cổng serial ảo qua USB (CDC-ACM). Cùng interface serial với UART nhưng
 * hành vi bên dưới khác hẳn: gửi theo gói, và chỉ gửi được khi host đã mở cổng.
 *
 * Field serial cố ý KHÔNG đặt đầu struct: CONTAINER_OF không cần điều đó.
 */
struct usb_cdc {
	uint8_t ep_in;                   /* địa chỉ endpoint IN, ví dụ 0x81 */
	bool connected;                  /* host đã mở cổng (DTR) chưa */
	const struct serial_ops *serial;
	uint32_t packets_sent;
	struct fifo rx;
};

int usb_cdc_init(struct usb_cdc *self, uint8_t ep_in);
serial_t usb_cdc_to_serial(struct usb_cdc *self);
/* Mô phỏng sự kiện từ USB stack */
void usb_cdc_set_connected(struct usb_cdc *self, bool connected);
void usb_cdc_host_send(struct usb_cdc *self, const uint8_t *data, size_t len);

#endif /* USB_CDC_H */
