#ifndef LM35_H
#define LM35_H

#include <stdint.h>

#include "sensor_api.h"

struct lm35 {
	struct sensor base; /* phải là field đầu tiên */
	uint8_t adc_channel;
	uint16_t vref_mv;
};

int lm35_init(struct lm35 *self, const char *name, uint8_t adc_channel, uint16_t vref_mv);

#endif /* LM35_H */
