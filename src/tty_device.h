#ifndef TTY_DEVICE_H__
#define TTY_DEVICE_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct tty_device_t tty_device_t;

tty_device_t *tty_open();
void tty_close(tty_device_t *td);
int tty_enable_raw(tty_device_t *td);
int tty_disable_raw(tty_device_t *td);
int tty_readable(tty_device_t *td, int ms);
int tty_read(tty_device_t *td, char *buf, int len);
int tty_write(tty_device_t *td, char *buf, int len);
int tty_is_backspace(tty_device_t *td, unsigned char c);
int tty_get_window_size(tty_device_t *td, int *row, int *col);

#ifdef __cplusplus
}
#endif

#endif
