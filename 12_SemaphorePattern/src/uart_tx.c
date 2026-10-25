#include "uart_tx.h"

#include "sim_cpu.h"

#include <errno.h>
#include <string.h>

int uart_tx_init(struct uart_tx *self, const char *name, uint32_t baudrate)
{
	if (!self || !name || baudrate == 0) {
		return -EINVAL;
	}

	memset(self, 0, sizeof(*self));
	self->name = name;
	/* 1 start bit + 8 data bit + 1 stop bit = 10 bit mỗi byte */
	self->byte_time_us = 10u * 1000000u / baudrate;
	/* Binary semaphore: ban đầu 0 (chưa có gì xong), tối đa 1 */
	return sem_init(&self->done, 0, 1);
}

/* "Ngắt TX complete": phần cứng gọi khi FIFO đã ra hết dây */
static void uart_tx_isr(void *arg)
{
	struct uart_tx *self = arg;

	/* Đọc "thanh ghi trạng thái" xem lần gửi này có lỗi không */
	self->status = self->fault ? -EIO : 0;
	self->fault = false;
	self->fifo_len = 0;

	/* Give trên MỌI nhánh, kể cả nhánh lỗi. Chỉ give khi thành công là lỗi
	 * "semaphore never given": thread đang chờ sẽ không bao giờ tỉnh. */
	sem_give(&self->done);
}

int uart_tx_transmit(struct uart_tx *self, const uint8_t *data, size_t len, int32_t timeout_ms)
{
	if (!self || !data) {
		return -EINVAL;
	}
	if (len > UART_TX_FIFO_SIZE) {
		return -ENOSPC;
	}

	/* (1) Nạp thanh ghi: chép dữ liệu vào FIFO và "bấm START" */
	memcpy(self->fifo, data, len);
	self->fifo_len = len;
	self->status = -EINPROGRESS;

	struct sim_irq_config hw = {
		.cpu_id = UART_TX_IRQ_CPU,
		.preempt = false,
		.isr = uart_tx_isr,
		.arg = self,
		.fire_count = 1,
		.gap_us = (uint32_t)len * self->byte_time_us, /* ngắt đến khi byte cuối ra dây */
	};
	int ret = sim_irq_start(&hw);
	if (ret) {
		return ret;
	}

	/* (2) Ngủ tới khi ISR give. Không polling: CPU làm việc khác trong lúc này */
	ret = sem_take(&self->done, timeout_ms);

	sim_irq_wait(NULL); /* dọn thread phần cứng giả lập */

	if (ret) {
		/* Hết giờ nhưng ngắt có thể vẫn đến muộn và give. Xóa tín hiệu muộn đó,
		 * nếu không lần transmit sau sẽ "tỉnh" ngay mà phần cứng chưa xong
		 * (Zephyr dùng k_sem_reset cho việc này). */
		sem_take(&self->done, K_NO_WAIT);
		return ret;
	}

	/* (3) Tiếp tục: status do ISR ghi, đọc được an toàn vì give/take đi qua
	 * spinlock của semaphore (release ở give, acquire ở take) */
	return self->status;
}

void uart_tx_inject_fault(struct uart_tx *self)
{
	if (self) {
		self->fault = true;
	}
}
