#ifndef SIM_CPU_H
#define SIM_CPU_H

#include <stdbool.h>
#include <stdint.h>

/*
 * Bộ mô phỏng "vi điều khiển" chạy trên PC (chỉ dùng cho demo).
 *
 * - CPU giả lập: một thread của PC gọi sim_cpu_bind(id) để trở thành CPU số id.
 *   Mỗi CPU có primask riêng (xem arch_irq.h).
 * - Nguồn ngắt giả lập: một thread "phần cứng" phát ngắt fire_count lần.
 *   Có hai chế độ:
 *     preempt = true : giống MCU đơn nhân. Thread phần cứng TẠM DỪNG thread của
 *                      CPU đích (SuspendThread của Windows) ở một lệnh máy bất kỳ,
 *                      chạy ISR, rồi cho chạy tiếp. Nếu CPU đích đang che ngắt,
 *                      ngắt chuyển sang "pending" và chạy ngay khi CPU mở ngắt.
 *     preempt = false: ISR chạy song song trên thread phần cứng, coi như được
 *                      CPU số cpu_id (một core khác) xử lý.
 *
 * Bộ mô phỏng chỉ có một nguồn ngắt tại một thời điểm (nó là một singleton
 * của "phần cứng"). Phần preempt chỉ cài đặt cho Windows.
 */

#define SIM_CPU_MAX 8

struct sim_irq_config {
	unsigned int cpu_id;   /* CPU xử lý ngắt */
	bool preempt;          /* true: ngắt "chen ngang" thread của cpu_id */
	void (*isr)(void *arg);
	void *arg;
	uint32_t fire_count;   /* số lần phần cứng phát ngắt */
	uint32_t gap_us;       /* khoảng nghỉ giữa hai lần phát ngắt */
};

/* Thread gọi hàm này trở thành CPU số cpu_id (0 <= cpu_id < SIM_CPU_MAX) */
int sim_cpu_bind(unsigned int cpu_id);

/* Bắt đầu phát ngắt (tạo thread phần cứng); trả -EBUSY nếu đang có nguồn khác */
int sim_irq_start(const struct sim_irq_config *cfg);

/* true khi phần cứng đã phát xong và mọi ISR đã chạy xong */
bool sim_irq_done(void);

/* Chờ phần cứng xong; *handled = số lần ISR thật sự chạy */
int sim_irq_wait(uint32_t *handled);

/* Chờ bận khoảng us micro giây (không nhường CPU) */
void sim_busy_wait_us(uint32_t us);

#endif /* SIM_CPU_H */
