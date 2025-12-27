#include <app_poll_lib/app_poll_lib.h>
#include <zephyr/kernel.h>

int main(void) {
  app_poll_lib_init();

  while (1) {
    k_sleep(K_SECONDS(1));
  }
}
