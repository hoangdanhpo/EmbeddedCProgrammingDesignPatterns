#ifndef GPS_H
#define GPS_H

#include "serial.h"

#include <stdint.h>

/*
 * Module GPS: gửi lệnh NMEA/PMTK tới module GPS qua một cổng serial BẤT KỲ.
 * gps.h chỉ include serial.h: gps không biết, và không cần biết, cổng đó là
 * UART, USB hay RAM giả trong test.
 */
#define GPS_CMD_MAX_LEN 64

struct gps {
	serial_t serial;  /* cổng được "tiêm" vào qua gps_init (dependency injection) */
	uint32_t cmds_sent;
};

int gps_init(struct gps *self, serial_t serial);
/* body không có '$' và checksum, ví dụ "PMTK220,1000". Gửi "$PMTK220,1000*1F\r\n" */
int gps_send_cmd(struct gps *self, const char *body);

#endif /* GPS_H */
