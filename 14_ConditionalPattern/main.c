#include "condvar.h"
#include "mutex.h"
#include "sample_queue.h"
#include "sys_events.h"

#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <time.h>

#define SAMPLES 20

/* Mọi state của demo nằm trong một object, truyền con trỏ cho từng thread */
struct app {
	struct sys_events events;
	struct sample_queue queue;
	struct timespec t0;
	pthread_mutex_t print_lock; /* Chỉ để các dòng printf không chen nhau */
};

static void sleep_ms(long ms)
{
	struct timespec d = { ms / 1000, (ms % 1000) * 1000000L };
	nanosleep(&d, NULL);
}

static long elapsed_ms(const struct app *app)
{
	struct timespec now;

	timespec_get(&now, TIME_UTC);
	return (long)((now.tv_sec - app->t0.tv_sec) * 1000 + (now.tv_nsec - app->t0.tv_nsec) / 1000000);
}

static void log_line(struct app *app, const char *who, const char *what, long value)
{
	pthread_mutex_lock(&app->print_lock);
	printf("[%4ld ms] %-13s %s %ld\n", elapsed_ms(app), who, what, value);
	pthread_mutex_unlock(&app->print_lock);
}

/* ---------- 1. Cổng khởi động: broadcast cho nhiều thread chờ điều kiện khác nhau ---------- */

struct waiter_cfg {
	struct app *app;
	const char *name;
	uint32_t mask;
};

static void *waiter_thread(void *arg)
{
	struct waiter_cfg *cfg = arg;
	uint32_t wakeups = 0;

	int ret = sys_events_wait_all(&cfg->app->events, cfg->mask, 2000, &wakeups);
	log_line(cfg->app, cfg->name, ret == 0 ? "bat dau chay, so lan bi danh thuc =" : "HET GIO, ret =",
		 ret == 0 ? (long)wakeups : (long)ret);

	if (ret == 0 && cfg->mask == SYS_EVT_NET_UP) {
		/* ntp_client: có mạng thì đồng bộ giờ (mất ~50 ms), rồi báo mốc tiếp theo */
		sleep_ms(50);
		int woken = sys_events_set(&cfg->app->events, SYS_EVT_TIME_SYNCED);
		log_line(cfg->app, cfg->name, "set TIME_SYNCED, danh thuc", woken);
	}
	return NULL;
}

struct setter_cfg {
	struct app *app;
	const char *name;
	long delay_ms;
	uint32_t bits;
};

static void *setter_thread(void *arg)
{
	struct setter_cfg *cfg = arg;

	sleep_ms(cfg->delay_ms);
	int woken = sys_events_set(&cfg->app->events, cfg->bits);
	log_line(cfg->app, cfg->name, "set moc, danh thuc", woken);
	return NULL;
}

static void demo_boot_gate(struct app *app)
{
	struct waiter_cfg waiters[] = {
		{ app, "mqtt_client", SYS_EVT_CONFIG_LOADED | SYS_EVT_NET_UP | SYS_EVT_TIME_SYNCED },
		{ app, "data_logger", SYS_EVT_CONFIG_LOADED },
		{ app, "ntp_client", SYS_EVT_NET_UP },
	};
	struct setter_cfg setters[] = {
		{ app, "config_loader", 100, SYS_EVT_CONFIG_LOADED },
		{ app, "net_driver", 250, SYS_EVT_NET_UP },
	};
	pthread_t wt[3];
	pthread_t st[2];

	printf("=== 1. Cong khoi dong (broadcast) ===\n");
	timespec_get(&app->t0, TIME_UTC);
	for (int i = 0; i < 3; i++) {
		pthread_create(&wt[i], NULL, waiter_thread, &waiters[i]);
	}
	sleep_ms(20); /* Cho ba waiter kịp vào hàng đợi; nếu chưa kịp thì vòng while vẫn đúng */
	for (int i = 0; i < 2; i++) {
		pthread_create(&st[i], NULL, setter_thread, &setters[i]);
	}
	for (int i = 0; i < 2; i++) {
		pthread_join(st[i], NULL);
	}
	for (int i = 0; i < 3; i++) {
		pthread_join(wt[i], NULL);
	}
	printf("\n");
}

/* ---------- 2. Producer - consumer: signal cho từng mẫu, broadcast khi đóng ---------- */

struct consumer_cfg {
	struct app *app;
	const char *name;
	int count;
	long sum;
};

static void *consumer_thread(void *arg)
{
	struct consumer_cfg *cfg = arg;
	int32_t v;

	while (sample_queue_get(&cfg->app->queue, &v, MUTEX_FOREVER) == 0) {
		cfg->count++;
		cfg->sum += v;
	}
	return NULL; /* -EPIPE: producer đã đóng hàng đợi và hết mẫu */
}

static void *adc_thread(void *arg)
{
	struct app *app = arg;

	for (int32_t i = 1; i <= SAMPLES; i++) {
		sample_queue_put(&app->queue, i, MUTEX_FOREVER); /* Đầy (4 mẫu) thì ngủ chờ not_full */
	}
	sample_queue_close(&app->queue);
	return NULL;
}

static void demo_producer_consumer(struct app *app)
{
	struct consumer_cfg c[2] = { { app, "filter_a", 0, 0 }, { app, "filter_b", 0, 0 } };
	pthread_t ct[2];
	pthread_t pt;

	printf("=== 2. Producer - consumer (signal + close/broadcast) ===\n");
	for (int i = 0; i < 2; i++) {
		pthread_create(&ct[i], NULL, consumer_thread, &c[i]);
	}
	pthread_create(&pt, NULL, adc_thread, app);
	pthread_join(pt, NULL);
	for (int i = 0; i < 2; i++) {
		pthread_join(ct[i], NULL);
	}
	printf("%s: %d mau, %s: %d mau\n", c[0].name, c[0].count, c[1].name, c[1].count);
	printf("tong %d mau (mong doi %d), tong gia tri %ld (mong doi %d)\n", c[0].count + c[1].count, SAMPLES,
	       c[0].sum + c[1].sum, SAMPLES * (SAMPLES + 1) / 2);
	int32_t v;
	printf("get sau khi dong -> %d (-EPIPE = %d)\n\n", sample_queue_get(&app->queue, &v, 0), -EPIPE);
}

/* ---------- 3. Quy tắc dùng condvar_wait ---------- */

static void demo_rules(void)
{
	struct mutex m;
	struct condvar cv;

	printf("=== 3. Quy tac cua condvar_wait ===\n");
	mutex_init(&m);
	condvar_init(&cv);

	printf("wait khi KHONG giu mutex      -> %d (-EPERM = %d)\n", condvar_wait(&cv, &m, 10), -EPERM);

	mutex_lock(&m, MUTEX_FOREVER);
	mutex_lock(&m, MUTEX_FOREVER);
	printf("wait khi mutex bi khoa long   -> %d (-EDEADLK = %d)\n", condvar_wait(&cv, &m, 10), -EDEADLK);
	mutex_unlock(&m);

	printf("broadcast khi khong ai cho    -> %d thread duoc danh thuc (tin hieu mat)\n", condvar_broadcast(&cv));
	int ret = condvar_wait(&cv, &m, 50);
	printf("wait 50 ms sau do             -> %d (-EAGAIN = %d), van giu mutex: depth %d\n", ret, -EAGAIN,
	       mutex_lock_depth(&m));
	mutex_unlock(&m);

	condvar_deinit(&cv);
	mutex_deinit(&m);
}

int main(void)
{
	struct app app;

	pthread_mutex_init(&app.print_lock, NULL);
	sys_events_init(&app.events);
	sample_queue_init(&app.queue);

	demo_boot_gate(&app);
	demo_producer_consumer(&app);
	demo_rules();

	sample_queue_deinit(&app.queue);
	sys_events_deinit(&app.events);
	pthread_mutex_destroy(&app.print_lock);
	return 0;
}
