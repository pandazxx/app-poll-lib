#include <__MODULE_NAME__/__MODULE_NAME__.h>
#include <zephyr/kernel.h>

int main(void) {
  __MODULE_NAME___init();

  int result = __MODULE_NAME___do_something(21);
  printk("Result = %d\n", result);

  while (1) {
    k_sleep(K_SECONDS(1));
  }
}
