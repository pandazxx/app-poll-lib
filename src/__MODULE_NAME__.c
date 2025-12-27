
#include <__MODULE_NAME__/__MODULE_NAME__.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(__MODULE_NAME__, LOG_LEVEL_INF);

int __MODULE_NAME___init(void) {
  LOG_INF("__MODULE_NAME__ init");
  return 0;
}

int __MODULE_NAME___do_something(int value) {
  LOG_INF("Doing something with %d", value);
  return value * 2;
}
