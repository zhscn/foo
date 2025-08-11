#include "tty_device.h"

#include <fcntl.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>

typedef struct tty_device_s {
  struct termios term;
  int fd;
  int raw;
} tty_device_s;

tty_device_t tty_open() {
  int fd = -1;
  tty_device_s *td = NULL;
  int ret = 0;

  fd = open("/dev/tty", O_RDWR);
  if (fd == -1) {
    goto clean;
  }

  td = (tty_device_s *)malloc(sizeof(tty_device_s));
  if (td == NULL) {
    goto clean;
  }

  ret = tcgetattr(fd, &td->term);
  if (ret == -1) {
    goto clean;
  }

  td->fd = fd;
  td->raw = 0;

  return td;

clean:
  if (fd != -1) {
    close(fd);
  }
  if (td != NULL) {
    free(td);
  }
  return NULL;
}

void tty_close(tty_device_t ptr) {
  tty_device_s *td = (tty_device_s *)ptr;

  if (td->fd != -1) {
    tty_disable_raw(td);
    close(td->fd);
    td->fd = -1;
  }
  free(td);
}

int tty_enable_raw(tty_device_t ptr) {
  tty_device_s *td = (tty_device_s *)ptr;

  struct termios t = td->term;
  t.c_iflag &=
      ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
  t.c_oflag &= ~OPOST;
  t.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
  t.c_lflag |= NOFLSH;
  t.c_cflag &= ~(CSIZE | PARENB);
  t.c_cflag |= CS8;
  t.c_cc[VMIN] = 0;
  t.c_cc[VTIME] = 0;

  int ret = tcsetattr(td->fd, TCSANOW, &t);
  if (ret == -1) {
    return -1;
  }

  td->raw = 1;
  return 0;
}

int tty_disable_raw(tty_device_t ptr) {
  tty_device_s *td = (tty_device_s *)ptr;

  if (td->raw == 0) {
    return 0;
  }

  int ret = tcsetattr(td->fd, TCSANOW, &td->term);
  if (ret == -1) {
    return -1;
  }

  td->raw = 0;
  return 0;
}

int tty_readable(tty_device_t ptr, int ms) {
  tty_device_s *td = (tty_device_s *)ptr;
  fd_set read_fds;
  FD_ZERO(&read_fds);
  FD_SET(td->fd, &read_fds);
  struct timeval timeout = {.tv_sec = ms / 1000,
                            .tv_usec = (ms % 1000) * 1000L};
  return select(td->fd + 1, &read_fds, NULL, NULL, &timeout) > 0;
}

int tty_read(tty_device_t ptr, char *buf, int len) {
  tty_device_s *td = (tty_device_s *)ptr;
  return read(td->fd, buf, len);
}

int tty_write(tty_device_t ptr, char *buf, int len) {
  tty_device_s *td = (tty_device_s *)ptr;
  return write(td->fd, buf, len);
}

int tty_is_backspace(tty_device_t ptr, char c) {
  tty_device_s *td = (tty_device_s *)ptr;
  return c == td->term.c_cc[VERASE];
}

int tty_get_window_size(tty_device_t ptr, int *row, int *col) {
  tty_device_s *td = (tty_device_s *)ptr;
  struct winsize ws;
  if (ioctl(td->fd, TIOCGWINSZ, &ws) != 0) {
    return -1;
  }
  *row = ws.ws_row;
  *col = ws.ws_col;
  return 0;
}
