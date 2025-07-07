#define BSD_SOURCE

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/types.h>
#include <sys/user.h>
#include <unistd.h>

int get_tty(char *out, size_t *len) {
  pid_t pid = getpid();
  struct kinfo_proc proc_info;
  size_t proc_len = sizeof(proc_info);
  int mib[4] = {CTL_KERN, KERN_PROC, KERN_PROC_PID, pid};

  if (sysctl(mib, 4, &proc_info, &proc_len, NULL, 0) == -1) {
    return -1;
  }

  dev_t tty_dev = proc_info.kp_eproc.e_tdev;

  if (tty_dev == NODEV) {
    return -1;
  }

  char tty_name_buf[256] = {0};
  char dev_prefix[] = "/dev/";
  strncpy(tty_name_buf, dev_prefix, sizeof(dev_prefix) - 1);

  if (devname_r(tty_dev, S_IFCHR, tty_name_buf + 5, sizeof(tty_name_buf) - 5) ==
      NULL) {
    return -1;
  }

  size_t tty_path_len = strlen(tty_name_buf);

  if (*len <= tty_path_len) {
    *len = tty_path_len + 1;
    return -1;
  }

  strncpy(out, tty_name_buf, tty_path_len);
  out[tty_path_len] = '\0';
  *len = tty_path_len;

  return 0;
}

void print_colored_hello(const char *tty_path) {
  int fd = open(tty_path, O_WRONLY);
  if (fd == -1) {
    perror("open TTY failed");
    return;
  }

  const char *colored_message =
      "\033[1;32mHello, \033[1;31mWorld!\033[1;34m from ";

  write(fd, colored_message, strlen(colored_message));
  write(fd, tty_path, strlen(tty_path));
  write(fd, "!\033[0m\n", 6);
  close(fd);
}

int main() {
  char tty_path[512];
  size_t len = sizeof(tty_path);

  if (get_tty(tty_path, &len) == 0) {
    print_colored_hello(tty_path);
  }

  return 0;
}
