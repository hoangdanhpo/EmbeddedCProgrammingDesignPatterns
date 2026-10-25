#include "gps.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

/* Chỉ include serial.h (qua gps.h): không có uart.h, usb_cdc.h hay serial_ops.h */

int gps_init(struct gps *self, serial_t serial)
{
	if (!self || !serial) {
		return -EINVAL;
	}

	memset(self, 0, sizeof(*self));
	self->serial = serial;
	return 0;
}

/* Checksum NMEA: XOR mọi ký tự nằm giữa '$' và '*' */
static uint8_t gps_nmea_checksum(const char *body)
{
	uint8_t sum = 0;

	while (*body) {
		sum ^= (uint8_t)*body++;
	}
	return sum;
}

int gps_send_cmd(struct gps *self, const char *body)
{
	char line[GPS_CMD_MAX_LEN];
	int len;
	int ret;

	if (!self || !body) {
		return -EINVAL;
	}

	len = snprintf(line, sizeof(line), "$%s*%02X\r\n", body, (unsigned)gps_nmea_checksum(body));
	if (len < 0 || (size_t)len >= sizeof(line)) {
		return -EMSGSIZE;
	}

	/* Lời gọi ảo: chạy code của UART, USB hay RAM tùy handle đã được tiêm vào */
	ret = serial_write(self->serial, (const uint8_t *)line, (size_t)len);
	if (ret < 0) {
		return ret;
	}
	if (ret != len) {
		/* Hợp đồng cho phép ghi thiếu; lệnh GPS bị cắt thì coi như lỗi */
		return -EIO;
	}
	self->cmds_sent++;
	return 0;
}
