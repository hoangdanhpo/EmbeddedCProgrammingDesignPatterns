#ifndef ARCH_IRQ_H
#define ARCH_IRQ_H

#include <stdbool.h>

/*
 * Lớp "kiến trúc" (arch): thao tác với mặt nạ ngắt của CPU đang chạy.
 *
 * Trên Cortex-M thật, arch_irq_lock() là hai lệnh "mrs key, PRIMASK; cpsid i"
 * và arch_irq_unlock() là "msr PRIMASK, key". Trên PC không có PRIMASK, nên
 * src/sim_cpu.c mô phỏng: mỗi CPU giả lập có một biến primask riêng, và nguồn
 * ngắt giả lập tôn trọng biến đó (ngắt đến lúc bị che sẽ "pending", chạy ngay
 * khi mở lại).
 *
 * Mọi hàm ở đây đều tác động lên CPU MÀ THREAD GỌI ĐANG ĐẠI DIỆN
 * (xem sim_cpu_bind()). Che ngắt luôn chỉ có tác dụng trên CPU hiện tại.
 */

/* Che ngắt trên CPU hiện tại; trả về trạng thái cũ (0 = trước đó đang mở) */
unsigned int arch_irq_lock(void);

/* Khôi phục đúng trạng thái đã lưu: chỉ mở ngắt nếu key == 0 */
void arch_irq_unlock(unsigned int key);

/* true nếu ngắt trên CPU hiện tại đang bị che */
bool arch_irq_is_locked(void);

/* true nếu code đang chạy trong ngữ cảnh ISR */
bool arch_is_in_isr(void);

/* Số hiệu CPU hiện tại (0, 1, ...) */
unsigned int arch_curr_cpu_id(void);

#endif /* ARCH_IRQ_H */
