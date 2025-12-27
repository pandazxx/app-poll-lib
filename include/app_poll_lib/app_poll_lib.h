
#ifndef APP_POLL_LIB_H_
#define APP_POLL_LIB_H_

#include <zephyr/kernel.h>
#include <zephyr/posix/poll.h>


#ifdef __cplusplus
extern "C" {
#endif

struct PollLoop;
typedef void (*fd_handler_t)(int fd, short revents, void *ud,
                             struct PollLoop *L);
typedef void (*poll_timeout_handler)();
int app_poll_lib_init(void);
int app_poll_lib_do_something(int value);

int register_poll_fd(int fd, fd_handler_t handler, short events,
                     void *user_data);

int deregister_poll_fd(int fd);

// void app_poll_init();
void app_poll_start();
void app_poll_loop();

poll_timeout_handler
app_register_poll_timeout_handler(poll_timeout_handler new_handler);

#ifdef __cplusplus
}
#endif

#endif
