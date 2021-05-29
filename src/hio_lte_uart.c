#include <hio_lte_uart.h>
#include <hio_bsp.h>

// Zephyr includes
#include <device.h>
#include <devicetree.h>
#include <drivers/uart.h>

#include <zephyr.h>

// Standard includes
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define TX_LINE_MAX_SIZE 1024
#define RX_LINE_MAX_SIZE 1024

#define RX_TIMEOUT 10

// Handle for UART
static const struct device *dev;

// Receive interrupt double buffer
static uint8_t __aligned(4) rx_buffer[2][256];

// Ring buffer object for received characters
static hio_sys_rbuf_t rx_ring_buf;

// Ring buffer memory for received characters
static uint8_t __aligned(4) rx_ring_buf_mem[1024];

// Heap object for received line buffers
static hio_sys_heap_t rx_heap;

// Heap memory for received line buffers
static char __aligned(4) rx_heap_mem[4 * RX_LINE_MAX_SIZE];

// Message queue object for pointers to received lines
static hio_sys_msgq_t rx_msgq;

// Message queue memory for pointers to received lines
static char __aligned(4) rx_msgq_mem[64 * sizeof(char *)];

// Semaphore indicating finished transmission
static hio_sys_sem_t tx_sem;

// Semaphore indicating received characters
static hio_sys_sem_t rx_sem;

// Task object for receiver worker
static hio_sys_task_t rx_task;

// Task memory for receiver worker
static HIO_SYS_TASK_STACK_DEFINE(rx_task_stack, 1024);

static void
uart_callback(const struct device *dev,
              struct uart_event *evt, void *user_data)
{
    static uint8_t *next_buf = rx_buffer[1];
    uint8_t *p;

    switch (evt->type) {
    case UART_TX_DONE:
        hio_sys_sem_give(&tx_sem);
        break;
    case UART_TX_ABORTED:
        // TODO Consider better handling/signalization?
        hio_sys_sem_give(&tx_sem);
        break;
    case UART_RX_RDY:
        p = evt->data.rx.buf + evt->data.rx.offset;
        hio_sys_rbuf_put(&rx_ring_buf, p, evt->data.rx.len);
        hio_sys_sem_give(&rx_sem);
        break;
    case UART_RX_BUF_REQUEST:
        uart_rx_buf_rsp(dev, next_buf, sizeof(rx_buffer[0]));
        break;
	case UART_RX_BUF_RELEASED:
		next_buf = evt->data.rx_buf.buf;
        break;
    case UART_RX_DISABLED:
        break;
    case UART_RX_STOPPED:
        // TODO Handle this
        break;
    }
}

static void
process_rx_char(char c)
{
    // Buffer for received characters
    static char buf[RX_LINE_MAX_SIZE];

    // Number of received characters
    static size_t len;

    // Indicates buffer is clipped
    static bool clipped;

    // Received new line character?
    if (c == '\r' || c == '\n') {

        // If buffer is not clipped and contains something...
        if (!clipped && len > 0) {

            if (buf[0] != '+') {
                printf("UART RX: %s\n", buf);

                // Allocate buffer from heap
                char *p = hio_sys_heap_alloc(&rx_heap, len, HIO_SYS_NO_WAIT);

                // Memory allocation failed?
                if (p == NULL) {
                    // TODO Log error
                } else {

                    // Copy line to allocated buffer
                    memcpy(p, buf, len + 1);

                    // Store pointer to buffer to RX queue
                    if (hio_sys_msgq_put(&rx_msgq, &p, HIO_SYS_NO_WAIT) < 0) {
                        // TODO Log error
                    }
                }
            } else {
                printf("URC: %s\n", buf);
            }
        }

        // TODO Not needed, just for debug
        memset(buf, 0, sizeof(buf));

        // Reset line
        len = 0;

        // Reset clip indicator
        clipped = false;

    } else {

        // Check for insufficient room...
        if (len >= sizeof(buf) - 1) {

            // Indicate clipped buffer
            clipped = true;

        } else {

            // Store character to buffer
            buf[len++] = c;
            buf[len] = '\0';
        }
    }
}

static void
rx_task_entry(void *param)
{
    for (;;) {
        // Wait for semaphore...
        if (hio_sys_sem_take(&rx_sem, HIO_SYS_FOREVER) < 0) {
            // TODO Should never get here
            continue;
        }

        // Process all available characters in ring buffer
        for (char c; hio_sys_rbuf_get(&rx_ring_buf, &c, 1) > 0;) {
            process_rx_char(c);
        }
    }
}

int
hio_lte_uart_init(void)
{
    // Initialize RX ring buffer
    hio_sys_rbuf_init(&rx_ring_buf, rx_ring_buf_mem, sizeof(rx_ring_buf_mem));

    // Initialize heap
    hio_sys_heap_init(&rx_heap, rx_heap_mem, sizeof(rx_heap_mem));

    // Initialize RX queue
    hio_sys_msgq_init(&rx_msgq, rx_msgq_mem, sizeof(char *), 64);

    hio_sys_sem_init(&tx_sem, 0);
    hio_sys_sem_init(&rx_sem, 0);
    hio_sys_task_init(&rx_task, rx_task_stack, HIO_SYS_TASK_STACK_SIZEOF(rx_task_stack), rx_task_entry, NULL);

    dev = device_get_binding("UART_0");

    if (dev == NULL) {
        return -1;
    }

    struct uart_config cfg = {
        .baudrate = 115200,
        .parity = UART_CFG_PARITY_NONE,
        .stop_bits = UART_CFG_STOP_BITS_1,
        .data_bits = UART_CFG_DATA_BITS_8,
        .flow_ctrl = UART_CFG_FLOW_CTRL_NONE
    };

    if (uart_configure(dev, &cfg) < 0) {
        return -2;
    }

    if (uart_callback_set(dev, uart_callback, NULL) < 0) {
        return -3;
    }

    if (uart_rx_enable(dev, rx_buffer[0], sizeof(rx_buffer[0]), RX_TIMEOUT) < 0) {
        return -4;
    }

    return 0;
}

int
hio_lte_uart_send(const char *fmt, va_list ap)
{
    static uint8_t __aligned(4) buf[TX_LINE_MAX_SIZE];

    int ret = vsnprintf(buf, sizeof(buf) - 2, fmt, ap);

    if (ret < 0) {
        return -1;
    } else if (ret > sizeof(buf) - 2) {
        return -2;
    }

    strcat(buf, "\r\n");

    printf("UART TX: %s", buf);

    if (uart_tx(dev, buf, strlen(buf), SYS_FOREVER_MS) < 0) {
        return -3;
    }

    // TODO Shall we set some reasonable timeout?
    if (hio_sys_sem_take(&tx_sem, HIO_SYS_FOREVER) < 0) {
        return -4;
    }

    return ret;
}

char *
hio_lte_uart_recv(hio_sys_timeout_t timeout)
{
    static uint8_t __aligned(4) buf[RX_LINE_MAX_SIZE];

    char *p;

    if (hio_sys_msgq_get(&rx_msgq, &p, timeout) < 0) {
        return NULL;
    }

    strcpy(buf, p);

    hio_sys_heap_free(&rx_heap, p);

    return buf;
}
