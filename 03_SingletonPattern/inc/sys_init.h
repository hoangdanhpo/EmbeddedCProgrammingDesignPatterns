#ifndef SYS_INIT_H
#define SYS_INIT_H

/*
 * Mô phỏng macro SYS_INIT của Zephyr trên PC.
 *
 * Zephyr đặt một entry vào bảng khởi tạo (một section riêng trong file thực
 * thi), và code khởi động duyệt bảng đó trước khi gọi main(). Ở đây ta mượn
 * đúng cơ chế mà C runtime dùng cho constructor: __attribute__((constructor(prio)))
 * đặt con trỏ hàm vào bảng init của chương trình, và runtime gọi chúng theo
 * thứ tự priority tăng dần, trước main().
 *
 * Priority 0..100 do gcc giữ lại cho runtime, nên ta dùng từ 101 trở lên.
 * Hàm init trả int giống Zephyr; ở bản mô phỏng này mã lỗi bị bỏ qua.
 */
#define SYS_INIT_PRIO_SUBSYSTEM 110 /* dịch vụ hệ thống: logger, settings... */
#define SYS_INIT_PRIO_BOARD     120 /* cấu hình board, dùng được các dịch vụ trên */

#define SYS_INIT(fn, prio)                                                    \
	static void __attribute__((constructor(prio))) fn##_sys_init_entry(void) \
	{                                                                     \
		(void)fn();                                                   \
	}

#endif /* SYS_INIT_H */
