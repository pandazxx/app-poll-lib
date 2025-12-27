
#include <app_poll_lib/app_poll_lib.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(app_poll_lib, LOG_LEVEL_INF);

int app_poll_lib_init(void) {
  LOG_INF("app_poll_lib init");
  return 0;
}

int app_poll_lib_do_something(int value) {
  LOG_INF("Doing something with %d", value);
  return value * 2;
}
