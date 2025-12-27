
#include "zephyr/sys/__assert.h"
#include <app_poll_lib/app_poll_lib.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(app_poll_lib, LOG_LEVEL_INF);

struct poll_reg {
  int fd; // -1 means free
  short events;
  fd_handler_t handler;
  void *ud;
};

struct PollLoop {
  struct poll_reg regs[CONFIG_APP_POLL_FDS_COUNT];
  int timeout_ms;
};

static struct PollLoop pollLoop;

static poll_timeout_handler app_poll_timeout_handler = NULL;
static bool initialized = false;

// ---------------- API ----------------

int app_poll_lib_init(void) {
  LOG_INF("app_poll_lib init");
  int timeout_ms = 2000;
  for (int i = 0; i < CONFIG_APP_POLL_FDS_COUNT; ++i) {
    pollLoop.regs[i].fd = -1;
  }
  pollLoop.timeout_ms = timeout_ms;
  initialized = true;
  return 0;
}

// void app_poll_init() {}

poll_timeout_handler
app_register_poll_timeout_handler(poll_timeout_handler new_handler) {
  poll_timeout_handler prev_handler = app_poll_timeout_handler;
  app_poll_timeout_handler = new_handler;
  return prev_handler;
}

int register_poll_fd(int fd, fd_handler_t handler, short events,
                     void *user_data) {
  __ASSERT(initialized, "app-poll-lib is not initialized");
  if (fd < 0 || !handler) {
    return -1;
  }
  for (int i = 0; i < CONFIG_APP_POLL_FDS_COUNT; ++i) {
    if (pollLoop.regs[i].fd < 0) {

      pollLoop.regs[i].fd = fd;
      pollLoop.regs[i].events = events;
      pollLoop.regs[i].handler = handler;
      pollLoop.regs[i].ud = user_data;
      return i;
    }
  }
  return -1;
}

// static int loop_add(int fd, short events, fd_handler_t h, void *ud) {
//   if (fd < 0 || !h) {
//     return -1;
//   }
//   for (int i = 0; i < CONFIG_APP_POLL_FDS_COUNT; ++i) {
//     if (pollLoop.regs[i].fd == -1) {
//       pollLoop.regs[i].fd = fd;
//       pollLoop.regs[i].events = events;
//       pollLoop.regs[i].handler = h;
//       pollLoop.regs[i].ud = ud;
//       return 0;
//     }
//   }
//   return -1; // full
// }
int deregister_poll_fd(int fd) {
  __ASSERT(initialized, "app-poll-lib is not initialized");
  for (int i = 0; i < CONFIG_APP_POLL_FDS_COUNT; ++i) {
    if (pollLoop.regs[i].fd == fd) {
      pollLoop.regs[i].fd = -1;
      return i;
    }
  }
  return -1;
}

// static int loop_del(int fd) {
//   for (int i = 0; i < CONFIG_APP_POLL_FDS_COUNT; ++i) {
//     if (pollLoop.regs[i].fd == fd) {
//       pollLoop.regs[i].fd = -1;
//       return 0;
//     }
//   }
//   return -1;
// }

// One-shot model: if an fd fires, we invoke its handler once and then remove
// it. Handlers can call loop_add() to add more fds.
// static void loop_run() {
void app_poll_loop() {
  __ASSERT(initialized, "app-poll-lib is not initialized");
  struct pollfd pfds[CONFIG_APP_POLL_FDS_COUNT];
  int idxmap[CONFIG_APP_POLL_FDS_COUNT]; // map poll slot ->
                                         // regs index

  int n = 0;
  for (int i = 0; i < CONFIG_APP_POLL_FDS_COUNT; ++i) {
    if (pollLoop.regs[i].fd != -1) {
      pfds[n].fd = pollLoop.regs[i].fd;
      pfds[n].events = pollLoop.regs[i].events;
      pfds[n].revents = 0;
      idxmap[n] = i;
      ++n;
    }
  }

  if (n == 0) {
    // Nothing to watch; simple sleep via poll with empty set & timeout
    (void)poll(NULL, 0, pollLoop.timeout_ms);
    return;
  }

  int ret = poll(pfds, (nfds_t)n, 0);
  if (ret < 0) {
    if (errno == EINTR)
      return;
    perror("poll");
    return;
  }
  if (ret == 0) {
    // TODO: handle timeout callback
    if (app_poll_timeout_handler != NULL) {
      app_poll_timeout_handler();
    }
    return;
  }

  // Handle ready fds; snapshot ensures safe add() in handlers
  for (int k = 0; k < n; ++k) {
    if (pfds[k].revents == 0)
      continue;

    int ri = idxmap[k];
    // Copy the registration (handler may mutate the table)
    int fd = pollLoop.regs[ri].fd;
    short evts = pfds[k].revents;
    fd_handler_t h = pollLoop.regs[ri].handler;
    void *ud = pollLoop.regs[ri].ud;

    if (fd == -1 || !h)
      continue;

    // Invoke once
    h(fd, evts, ud, &pollLoop);

#if APP_POLL_ONE_SHOT
    // Auto-cleanup after being handled
    // deregister_poll_fd(fd);
#endif
  }
}
void app_poll_start() {
  __ASSERT(initialized, "app-poll-lib is not initialized");
  struct pollfd pfds[CONFIG_APP_POLL_FDS_COUNT];
  int idxmap[CONFIG_APP_POLL_FDS_COUNT]; // map poll slot ->
                                         // regs index

  for (;;) {
    int n = 0;
    for (int i = 0; i < CONFIG_APP_POLL_FDS_COUNT; ++i) {
      if (pollLoop.regs[i].fd != -1) {
        pfds[n].fd = pollLoop.regs[i].fd;
        pfds[n].events = pollLoop.regs[i].events;
        pfds[n].revents = 0;
        idxmap[n] = i;
        ++n;
      }
    }

    if (n == 0) {
      // Nothing to watch; simple sleep via poll with empty set & timeout
      (void)poll(NULL, 0, pollLoop.timeout_ms);
      continue;
    }

    int ret = poll(pfds, (nfds_t)n, pollLoop.timeout_ms);
    if (ret < 0) {
      if (errno == EINTR)
        continue;
      perror("poll");
      break;
    }
    if (ret == 0) {
      // TODO: handle timeout callback
      if (app_poll_timeout_handler != NULL) {
        app_poll_timeout_handler();
      }
      continue; // timeout
    }

    // Handle ready fds; snapshot ensures safe add() in handlers
    for (int k = 0; k < n; ++k) {
      if (pfds[k].revents == 0)
        continue;

      int ri = idxmap[k];
      // Copy the registration (handler may mutate the table)
      int fd = pollLoop.regs[ri].fd;
      short evts = pfds[k].revents;
      fd_handler_t h = pollLoop.regs[ri].handler;
      void *ud = pollLoop.regs[ri].ud;

      if (fd == -1 || !h)
        continue;

      // Invoke once
      h(fd, evts, ud, &pollLoop);

#if APP_POLL_ONE_SHOT
      // Auto-cleanup after being handled
      // deregister_poll_fd(fd);
#endif
    }
  }
}
