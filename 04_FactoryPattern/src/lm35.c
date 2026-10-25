#include "lm35.h"
#include "board_sim.h"

#include <errno.h>
#include <string.h>

#define LM35_ADC_CHANNELS  16
#define LM35_ADC_FULL      4095 /* ADC 12 bit */
#define LM35_VREF_MIN_MV   1000
#define LM35_VREF_MAX_MV   3600
#define LM35_MILLI_C_PER_MV 100 /* 10 mV/°C nghĩa là 1 mV = 0.1 °C = 100 m°C */

static int lm35_read(sensor_t sensor, int32_t *out_milli_c)
{
	struct lm35 *self = CONTAINER_OF(sensor, struct lm35, base.api);
	uint16_t raw;
	int ret = board_adc_read(self->adc_channel, &raw);

	if (ret) {
		return ret;
	}

	/* Tính bằng uint32_t: 4095 * 3600 vẫn vừa, không tràn như khi dùng 16 bit */
	uint32_t mv = (uint32_t)raw * self->vref_mv / LM35_ADC_FULL;
	*out_milli_c = (int32_t)(mv * LM35_MILLI_C_PER_MV);
	return 0;
}

/* LM35 không có gì để tắt, nên deinit = NULL: factory phải chịu được trường hợp này */
static const struct sensor_api lm35_api = {
	.read = lm35_read,
	.deinit = NULL,
};

int lm35_init(struct lm35 *self, const char *name, uint8_t adc_channel, uint16_t vref_mv)
{
	if (!self || !name || adc_channel >= LM35_ADC_CHANNELS || vref_mv < LM35_VREF_MIN_MV ||
	    vref_mv > LM35_VREF_MAX_MV) {
		return -EINVAL;
	}

	memset(self, 0, sizeof(*self));
	self->base.api = &lm35_api;
	self->base.name = name;
	self->adc_channel = adc_channel;
	self->vref_mv = vref_mv;
	return 0;
}
