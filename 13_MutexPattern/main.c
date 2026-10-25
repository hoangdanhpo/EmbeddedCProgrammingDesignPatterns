#include "i2c_bus.h"
#include "mutex.h"
#include "tmp102.h"

#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <time.h>

#define SENSOR_ADDR 0x48
#define EEPROM_ADDR 0x50
#define ROUNDS 300

/* Mọi state của demo nằm trong một object, truyền con trỏ cho từng thread */
struct app {
	struct i2c_bus i2c1;
	struct i2c_bus i2c2;
	struct tmp102 sensor;
	int sensor_errors;
	int logger_errors;
};

static void sleep_ms(long ms)
{
	struct timespec d = { ms / 1000, (ms % 1000) * 1000000L };
	nanosleep(&d, NULL);
}

/* ---------- 1. Hai thread dùng chung một bus ---------- */

static void *sensor_thread(void *arg)
{
	struct app *app = arg;

	for (int i = 0; i < ROUNDS; i++) {
		int32_t mc = 0;
		/* Cảm biến mô phỏng luôn báo 25.000 độ C: đọc ra số khác nghĩa là đã đọc nhầm thiết bị */
		if (tmp102_read_mc(&app->sensor, &mc) != 0 || mc != 25000) {
			app->sensor_errors++;
		}
	}
	return NULL;
}

static void *logger_thread(void *arg)
{
	struct app *app = arg;

	for (int i = 0; i < ROUNDS; i++) {
		uint8_t cell = (uint8_t)i;
		uint8_t value = (uint8_t)(0x80u | (i & 0x7Fu)); /* Bit 7 luôn bật: khác hẳn byte của TMP102 */
		uint8_t back = 0;

		if (i2c_bus_write(&app->i2c1, EEPROM_ADDR, cell, &value, 1) != 0 ||
		    i2c_bus_write_read(&app->i2c1, EEPROM_ADDR, cell, &back, 1) != 0 || back != value) {
			app->logger_errors++;
		}
	}
	return NULL;
}

static void demo_shared_bus(struct app *app)
{
	pthread_t sensor;
	pthread_t logger;

	printf("=== 1. Hai thread dung chung bus %s ===\n", app->i2c1.name);
	pthread_create(&sensor, NULL, sensor_thread, app);
	pthread_create(&logger, NULL, logger_thread, app);
	pthread_join(sensor, NULL);
	pthread_join(logger, NULL);

	printf("sensor thread: %d lan doc TMP102, sai %d\n", ROUNDS, app->sensor_errors);
	printf("logger thread: %d lan ghi + doc lai EEPROM, sai %d\n", ROUNDS, app->logger_errors);
	printf("bus %s: %u transfer, lock phai cho %u lan\n\n", app->i2c1.name,
	       (unsigned)app->i2c1.transfers, (unsigned)app->i2c1.lock.contentions);
}

/* ---------- 2. Khóa lồng ---------- */

static void demo_recursive(struct app *app)
{
	struct mutex m;
	struct tmp102_snapshot snap;

	printf("=== 2. Khoa long (recursive) ===\n");
	mutex_init(&m);
	mutex_lock(&m, MUTEX_FOREVER);
	printf("lock lan 1 -> depth %d\n", mutex_lock_depth(&m));
	mutex_lock(&m, MUTEX_FOREVER);
	printf("lock lan 2 -> depth %d (cung thread: khong deadlock)\n", mutex_lock_depth(&m));
	mutex_unlock(&m);
	printf("unlock     -> depth %d (van dang giu)\n", mutex_lock_depth(&m));
	mutex_unlock(&m);
	printf("unlock     -> depth %d (-EPERM: khong con la owner)\n", mutex_lock_depth(&m));
	mutex_deinit(&m);

	int ret = tmp102_read_snapshot(&app->sensor, &snap);
	printf("tmp102_read_snapshot -> %d, config=0x%04X, temp=%ld mC\n\n", ret, (unsigned)snap.config,
	       (long)snap.milli_c);
}

/* ---------- 3. Mã lỗi: EBUSY, EAGAIN, EPERM, EINVAL ---------- */

struct holder_ctx {
	struct i2c_bus *bus;
	pthread_mutex_t ready_lock; /* Báo cho main biết holder đã giữ bus */
	pthread_cond_t ready_cv;
	int ready;
};

static void *holder_thread(void *arg)
{
	struct holder_ctx *ctx = arg;

	i2c_bus_lock(ctx->bus, MUTEX_FOREVER);
	pthread_mutex_lock(&ctx->ready_lock);
	ctx->ready = 1;
	pthread_cond_signal(&ctx->ready_cv);
	pthread_mutex_unlock(&ctx->ready_lock);

	sleep_ms(300); /* Giữ bus lâu, như một transfer EEPROM dài */
	i2c_bus_unlock(ctx->bus);
	return NULL;
}

static void demo_error_codes(struct app *app)
{
	struct holder_ctx ctx = { .bus = &app->i2c1, .ready = 0 };
	pthread_t holder;

	printf("=== 3. Ma loi ===\n");
	pthread_mutex_init(&ctx.ready_lock, NULL);
	pthread_cond_init(&ctx.ready_cv, NULL);
	pthread_create(&holder, NULL, holder_thread, &ctx);

	pthread_mutex_lock(&ctx.ready_lock);
	while (!ctx.ready) {
		pthread_cond_wait(&ctx.ready_cv, &ctx.ready_lock);
	}
	pthread_mutex_unlock(&ctx.ready_lock);

	printf("holder thread dang giu bus %s trong 300 ms\n", app->i2c1.name);
	printf("main: lock NO_WAIT        -> %d (-EBUSY = %d)\n", i2c_bus_lock(&app->i2c1, MUTEX_NO_WAIT), -EBUSY);
	printf("main: lock timeout 50 ms  -> %d (-EAGAIN = %d)\n", i2c_bus_lock(&app->i2c1, 50), -EAGAIN);
	printf("main: unlock khi khong giu -> %d (-EPERM = %d)\n", i2c_bus_unlock(&app->i2c1), -EPERM);

	pthread_join(holder, NULL);
	printf("main: unlock khi bus mo   -> %d (-EINVAL = %d)\n", i2c_bus_unlock(&app->i2c1), -EINVAL);
	printf("main: lock sau khi holder xong -> %d\n\n", i2c_bus_lock(&app->i2c1, MUTEX_NO_WAIT));
	i2c_bus_unlock(&app->i2c1);

	pthread_cond_destroy(&ctx.ready_cv);
	pthread_mutex_destroy(&ctx.ready_lock);
}

/* ---------- 4. Thứ tự khóa và deadlock ---------- */

struct order_ctx {
	const char *name;
	struct i2c_bus *first;
	struct i2c_bus *second;
	int result;
};

static void *order_thread(void *arg)
{
	struct order_ctx *ctx = arg;

	i2c_bus_lock(ctx->first, MUTEX_FOREVER);
	sleep_ms(50); /* Nới rộng cửa sổ để thread kia kịp giữ bus còn lại */
	/* Timeout chỉ để demo thoát ra được; firmware thật thường chờ FOREVER và treo vĩnh viễn */
	ctx->result = i2c_bus_lock(ctx->second, 200);
	if (ctx->result == 0) {
		i2c_bus_unlock(ctx->second);
	}
	i2c_bus_unlock(ctx->first);
	return NULL;
}

static void run_pair(struct order_ctx *a, struct order_ctx *b)
{
	pthread_t ta;
	pthread_t tb;

	pthread_create(&ta, NULL, order_thread, a);
	pthread_create(&tb, NULL, order_thread, b);
	pthread_join(ta, NULL);
	pthread_join(tb, NULL);
	printf("  %s: lock %s -> %s, lan 2 -> %d\n", a->name, a->first->name, a->second->name, a->result);
	printf("  %s: lock %s -> %s, lan 2 -> %d\n", b->name, b->first->name, b->second->name, b->result);
}

static void demo_lock_order(struct app *app)
{
	printf("=== 4. Thu tu khoa ===\n");

	printf("Nguoc thu tu (AB-BA):\n");
	struct order_ctx a = { "copy_task ", &app->i2c1, &app->i2c2, 0 };
	struct order_ctx b = { "calib_task", &app->i2c2, &app->i2c1, 0 };
	run_pair(&a, &b);

	printf("Cung thu tu (luon I2C1 truoc I2C2):\n");
	b.first = &app->i2c1;
	b.second = &app->i2c2;
	run_pair(&a, &b);
}

int main(void)
{
	struct app app = { .sensor_errors = 0, .logger_errors = 0 };

	i2c_bus_init(&app.i2c1, "I2C1");
	i2c_bus_init(&app.i2c2, "I2C2");
	tmp102_init(&app.sensor, &app.i2c1, SENSOR_ADDR);

	demo_shared_bus(&app);
	demo_recursive(&app);
	demo_error_codes(&app);
	demo_lock_order(&app);

	i2c_bus_deinit(&app.i2c2);
	i2c_bus_deinit(&app.i2c1);
	return 0;
}
