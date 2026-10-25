#ifndef SENSOR_H
#define SENSOR_H

#include <stdint.h>

/*
 * API chung cho MỌI loại cảm biến nhiệt độ (TMP102 qua I2C, LM35 qua ADC,
 * cảm biến giả cho test...). Code ứng dụng chỉ include file này.
 *
 * struct sensor_api chỉ được khai báo: ứng dụng không biết bên trong có gì.
 */
struct sensor_api;

/*
 * Handle: con trỏ tới field "api" nằm bên trong object cụ thể. Implementation
 * dùng CONTAINER_OF để từ handle lấy lại object của mình (xem src/sensor_api.h).
 * typedef theo khuyến nghị của sách cho kiểu handle, để khỏi viết "**" khắp nơi.
 */
typedef const struct sensor_api **sensor_t;

/* Nhiệt độ theo milli độ C (25.5 °C = 25500), tránh dùng float trên MCU không có FPU */
int sensor_read(sensor_t sensor, int32_t *out_milli_c);
const char *sensor_name(sensor_t sensor);

#endif /* SENSOR_H */
