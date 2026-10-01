#ifndef UART_HW_H
#define UART_HW_H

#include <stdint.h>

/*
 * Mô phỏng header của vendor/HAL. Header này nằm trong src/, không nằm
 * trong inc/: chỉ uart.c được include, main.c không thể nhìn thấy nó.
 */

#define UART_HW_CLOCK_HZ 16000000u

struct uart_hw_regs {
	volatile uint32_t BRR; /* Hệ số chia baud rate */
	volatile uint32_t DR;  /* Thanh ghi dữ liệu */
};

#endif /* UART_HW_H */
