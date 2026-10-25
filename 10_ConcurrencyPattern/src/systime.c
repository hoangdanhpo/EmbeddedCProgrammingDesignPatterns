#define _POSIX_C_SOURCE 200809L

#include "systime.h"

#include <time.h>

#ifdef _WIN32
#include <windows.h>
#ifndef CREATE_WAITABLE_TIMER_HIGH_RESOLUTION
#define CREATE_WAITABLE_TIMER_HIGH_RESOLUTION 0x00000002
#endif
#endif

uint32_t systime_ms(void)
{
	struct timespec ts;

	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (uint32_t)((uint64_t)ts.tv_sec * 1000u + (uint64_t)ts.tv_nsec / 1000000u);
}

#ifdef _WIN32
/*
 * Trên Windows, nanosleep(1 ms) thực tế ngủ ~15.6 ms vì "tick" mặc định của
 * hệ điều hành là 15.625 ms. Timer độ phân giải cao cho tick ~1 ms, gần với
 * tick 1 kHz của một RTOS. Mỗi thread một timer riêng để không giẫm lên nhau.
 */
void systime_sleep_ms(uint32_t ms)
{
	static _Thread_local HANDLE timer;
	LARGE_INTEGER due;

	if (!timer) {
		timer = CreateWaitableTimerExW(NULL, NULL, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION,
					       TIMER_ALL_ACCESS);
	}
	if (!timer) {
		Sleep(ms);
		return;
	}
	due.QuadPart = -(LONGLONG)ms * 10000; /* âm = tương đối, đơn vị 100 ns */
	SetWaitableTimer(timer, &due, 0, NULL, NULL, FALSE);
	WaitForSingleObject(timer, INFINITE);
}
#else
void systime_sleep_ms(uint32_t ms)
{
	struct timespec req = { (time_t)(ms / 1000u), (long)(ms % 1000u) * 1000000L };

	while (nanosleep(&req, &req) != 0) {
		/* bị tín hiệu đánh thức sớm: ngủ tiếp phần còn lại */
	}
}
#endif
