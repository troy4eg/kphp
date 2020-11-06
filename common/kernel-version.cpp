#include "common/kernel-version.h"

#include <sys/utsname.h>

#include "common/kprintf.h"
#include "common/stats/provider.h"

static struct utsname* cached_uname() {
  static struct utsname kernel_version;
  static int got_kernel_version = 0;
  if (got_kernel_version == 0) {
    int res = uname(&kernel_version);
    if (res == 0) {
      got_kernel_version = 1;
    } else {
      kprintf("fail getting kernel version: %m\n");
      got_kernel_version = -1;
    }
  }
  if (got_kernel_version < 0) {
    return NULL;
  }
  return &kernel_version;
}

static int kernel_x;
static int kernel_y;
static int kernel_z;

static inline void parse_kernel_version() {
  if (kernel_x) {
    return;
  }
  struct utsname *uname_val = cached_uname();
  if (!uname_val || sscanf(uname_val->release, "%d.%d.%d", &kernel_x, &kernel_y, &kernel_z) != 3) {
    kernel_x = kernel_y = kernel_z = -1;
  }
}

STATS_PROVIDER(kernel, 1000) {
  struct utsname *uname_val = cached_uname();
  if (uname_val != NULL) {
    add_general_stat(stats, "kernel_version", "%s", uname_val->release);
  }
}

int epoll_exclusive_supported() {
  parse_kernel_version();
  return kernel_x > 4 || (kernel_x == 4 && kernel_y >= 5);
}

int madvise_madv_free_supported() {
  parse_kernel_version();
  return kernel_x > 4 || (kernel_x == 4 && kernel_y >= 5);
}
