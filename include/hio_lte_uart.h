#ifndef HIO_LTE_UART_H
#define HIO_LTE_UART_H

#include <hio_sys.h>

int
hio_lte_uart_init(void);

int
hio_lte_uart_send(const char *fmt, va_list ap);

int
hio_lte_uart_recv(char **s, hio_sys_timeout_t timeout);

#endif
