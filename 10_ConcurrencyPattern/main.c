#include "heartbeat.h"
#include "sensor_task.h"
#include "slow_sensor.h"
#include "systime.h"
#include "work_queue.h"

#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>

#define DEMO_MS          2000u /* mỗi demo chạy 2 giây */
#define LED_PERIOD_MS    50u   /* LED nhịp tim đảo mỗi 50 ms */
#define SAMPLE_PERIOD_MS 500u  /* đọc nhiệt độ mỗi 500 ms */

static void print_heartbeat(const char *title, const struct heartbeat *hb)
{
	printf("%s\n", title);
	printf("  LED timeline (1 char = %u ms): %.*s\n", (unsigned)hb->period_ms,
	       (int)(DEMO_MS / hb->period_ms), hb->timeline);
	printf("  toggles: %u (expected ~%u), late: %u, worst gap: %u ms\n", (unsigned)hb->toggles,
	       (unsigned)(DEMO_MS / hb->period_ms - 1), (unsigned)hb->late,
	       (unsigned)hb->max_interval_ms);
}

/*
 * Demo 1: superloop. Một vòng lặp làm mọi thứ theo thứ tự. Lần đọc cảm biến
 * chặn 375 ms, và trong 375 ms đó không ai đảo LED.
 */
static int demo_superloop(void)
{
	struct heartbeat hb;
	struct slow_sensor sensor;
	uint32_t now = systime_ms();
	uint32_t start = now;
	uint32_t next_sample = now;
	unsigned samples = 0;
	int32_t mdeg = 0;

	if (heartbeat_init(&hb, LED_PERIOD_MS, now) < 0 ||
	    slow_sensor_init(&sensor, SLOW_SENSOR_CONVERSION_MS) < 0) {
		return -1;
	}

	while (!systime_reached(now = systime_ms(), start + DEMO_MS)) {
		if (heartbeat_poll(&hb, now) < 0) {
			return -1;
		}
		if (systime_reached(now, next_sample)) {
			next_sample += SAMPLE_PERIOD_MS;
			if (slow_sensor_read(&sensor, &mdeg) == 0) { /* chặn 375 ms ngay tại đây */
				samples++;
			}
		}
		systime_sleep_ms(1); /* "WFI": nhường CPU tới tick kế tiếp */
	}

	print_heartbeat("== Demo 1: superloop, sensor read inside the loop ==", &hb);
	printf("  sensor samples: %u, last = %ld m°C\n\n", samples, (long)mdeg);
	return 0;
}

/*
 * Demo 2: cùng vòng lặp, nhưng phần đọc cảm biến được đẩy sang work queue.
 * Vòng lặp chính chỉ submit (vài micro-giây) rồi quay lại lo LED.
 */
static int demo_work_queue(void)
{
	static struct work_queue wq;
	struct heartbeat hb;
	struct slow_sensor sensor;
	struct sensor_task task;
	uint32_t now = systime_ms();
	uint32_t start = now;
	uint32_t next_sample = now;
	unsigned already_pending = 0;
	int32_t mdeg = 0;

	if (heartbeat_init(&hb, LED_PERIOD_MS, now) < 0 ||
	    slow_sensor_init(&sensor, SLOW_SENSOR_CONVERSION_MS) < 0 ||
	    sensor_task_init(&task, &sensor) < 0 || work_queue_start(&wq) < 0) {
		return -1;
	}

	while (!systime_reached(now = systime_ms(), start + DEMO_MS)) {
		if (heartbeat_poll(&hb, now) < 0) {
			return -1;
		}
		if (systime_reached(now, next_sample)) {
			next_sample += SAMPLE_PERIOD_MS;
			int ret = work_queue_submit(&wq, &task.work); /* không chặn */

			if (ret < 0) {
				return -1;
			}
			if (ret == 1) {
				already_pending++; /* lần đọc trước chưa xong: bỏ qua, không xếp chồng */
			}
		}
		systime_sleep_ms(1);
	}

	if (work_queue_stop(&wq) < 0) {
		return -1;
	}

	print_heartbeat("== Demo 2: same loop, sensor read moved to a work queue thread ==", &hb);
	if (sensor_task_latest(&task, &mdeg) == 0) {
		printf("  sensor samples: %u, last = %ld m°C, work items run: %u, "
		       "skipped (already pending): %u\n\n",
		       atomic_load(&task.samples), (long)mdeg, wq.executed, already_pending);
	}
	return 0;
}

/*
 * Demo 3: cái giá của concurrency. Hai thread cùng tăng một biến chung.
 * counter++ là đọc - cộng - ghi; hai thread chen nhau thì mất lần tăng.
 */
#define RACE_ITERATIONS 1000000L

static volatile long plain_counter; /* volatile KHÔNG làm phép ++ thành nguyên tử */
static atomic_long atomic_counter;

static void *inc_plain(void *arg)
{
	(void)arg;
	for (long i = 0; i < RACE_ITERATIONS; i++) {
		plain_counter++;
	}
	return NULL;
}

static void *inc_atomic(void *arg)
{
	(void)arg;
	for (long i = 0; i < RACE_ITERATIONS; i++) {
		atomic_fetch_add(&atomic_counter, 1);
	}
	return NULL;
}

static int run_pair(void *(*fn)(void *))
{
	pthread_t a;
	pthread_t b;

	if (pthread_create(&a, NULL, fn, NULL) != 0) {
		return -1;
	}
	if (pthread_create(&b, NULL, fn, NULL) != 0) {
		pthread_join(a, NULL);
		return -1;
	}
	pthread_join(a, NULL);
	pthread_join(b, NULL);
	return 0;
}

static int demo_race(void)
{
	if (run_pair(inc_plain) < 0 || run_pair(inc_atomic) < 0) {
		return -1;
	}

	printf("== Demo 3: two threads, 1000000 increments each ==\n");
	printf("  plain counter : %ld (lost %ld updates)\n", plain_counter,
	       2 * RACE_ITERATIONS - plain_counter);
	printf("  atomic counter: %ld\n", atomic_load(&atomic_counter));
	return 0;
}

int main(void)
{
	if (demo_superloop() < 0 || demo_work_queue() < 0 || demo_race() < 0) {
		printf("demo failed\n");
		return 1;
	}
	return 0;
}
