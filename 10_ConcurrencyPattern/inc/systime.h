#ifndef SYSTIME_H
#define SYSTIME_H

#include <stdint.h>

/*
 * "Đồng hồ hệ thống" của bản mô phỏng trên PC. Trên MCU, hai hàm này tương
 * ứng với bộ đếm tick của RTOS (k_uptime_get_32(), k_msleep() trong Zephyr).
 */

/* Số mili-giây kể từ một mốc cố định; tràn sau ~49 ngày, so sánh bằng hiệu có dấu */
uint32_t systime_ms(void);

/* Nhường CPU ít nhất ms mili-giây (thread gọi bị chặn, thread khác được chạy) */
void systime_sleep_ms(uint32_t ms);

/* true nếu thời điểm a đã tới hoặc qua thời điểm b, đúng cả khi bộ đếm tràn */
static inline int systime_reached(uint32_t a, uint32_t b)
{
	return (int32_t)(a - b) >= 0;
}

#endif /* SYSTIME_H */
