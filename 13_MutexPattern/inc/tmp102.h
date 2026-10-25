#ifndef TMP102_H
#define TMP102_H

#include "i2c_bus.h"

#include <stdint.h>

/* Driver cảm biến nhiệt độ TMP102. Không có lock riêng: dùng lock của bus mà nó nằm trên. */
struct tmp102 {
	struct i2c_bus *bus;
	uint8_t addr;
};

struct tmp102_snapshot {
	uint16_t config;
	int32_t milli_c;
};

int tmp102_init(struct tmp102 *self, struct i2c_bus *bus, uint8_t addr);

/* Một transfer: đọc thanh ghi nhiệt độ, đổi sang mili độ C. */
int tmp102_read_mc(struct tmp102 *self, int32_t *milli_c);

/* Hai transfer (config + nhiệt độ) trong cùng một lần giữ bus: không ai chen vào giữa. */
int tmp102_read_snapshot(struct tmp102 *self, struct tmp102_snapshot *out);

#endif /* TMP102_H */
