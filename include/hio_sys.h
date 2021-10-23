#ifndef HIO_SYS_H
#define HIO_SYS_H

// Zephyr includes
#include <sys/ring_buffer.h>
#include <zephyr.h>

// Standard includes
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define HIO_SYS_FOREVER ((int64_t)0x7fffffffffffffff)
#define HIO_SYS_NO_WAIT ((int64_t)0)

#define HIO_SYS_MSEC(n) ((int64_t)(n))
#define HIO_SYS_SECONDS(n) ((int64_t)(n)*HIO_SYS_MSEC(1000))
#define HIO_SYS_MINUTES(n) ((int64_t)(n)*HIO_SYS_SECONDS(60))
#define HIO_SYS_HOURS(n) ((int64_t)(n)*HIO_SYS_MINUTES(60))
#define HIO_SYS_DAYS(n) ((int64_t)(n)*HIO_SYS_HOURS(24))

#define HIO_SYS_TASK_STACK_DEFINE K_THREAD_STACK_DEFINE
#define HIO_SYS_TASK_STACK_MEMBER K_THREAD_STACK_MEMBER
#define HIO_SYS_TASK_STACK_SIZEOF K_THREAD_STACK_SIZEOF

typedef struct k_heap hio_sys_heap_t;
typedef k_tid_t hio_sys_task_id_t;
typedef struct k_thread hio_sys_task_t;
typedef k_thread_stack_t hio_sys_task_stack_t;
typedef void (*hio_sys_task_entry_t)(void *param);
typedef struct k_sem hio_sys_sem_t;
typedef struct k_mutex hio_sys_mut_t;
typedef struct k_msgq hio_sys_msgq_t;
typedef struct ring_buf hio_sys_rbuf_t;

void hio_sys_reboot(void);
int64_t hio_sys_uptime_get(void);
void hio_sys_heap_init(hio_sys_heap_t *heap, void *mem, size_t mem_size);
void *hio_sys_heap_alloc(hio_sys_heap_t *heap, size_t bytes, int64_t timeout);
void hio_sys_heap_free(hio_sys_heap_t *heap, void *mem);
hio_sys_task_id_t hio_sys_task_init(hio_sys_task_t *task, const char *name,
                                    hio_sys_task_stack_t *stack, size_t stack_size,
                                    hio_sys_task_entry_t entry, void *param);
void hio_sys_task_sleep(int64_t timeout);
void hio_sys_sem_init(hio_sys_sem_t *sem, unsigned int value);
void hio_sys_sem_give(hio_sys_sem_t *sem);
int hio_sys_sem_take(hio_sys_sem_t *sem, int64_t timeout);
void hio_sys_mut_init(hio_sys_mut_t *mut);
void hio_sys_mut_acquire(hio_sys_mut_t *mut);
void hio_sys_mut_release(hio_sys_mut_t *mut);
void hio_sys_msgq_init(hio_sys_msgq_t *msgq, void *mem, size_t item_size, size_t max_items);
int hio_sys_msgq_put(hio_sys_msgq_t *msgq, const void *item, int64_t timeout);
int hio_sys_msgq_get(hio_sys_msgq_t *msgq, void *item, int64_t timeout);
void hio_sys_rbuf_init(hio_sys_rbuf_t *rbuf, void *mem, size_t mem_size);
size_t hio_sys_rbuf_put(hio_sys_rbuf_t *rbuf, const uint8_t *data, size_t bytes);
size_t hio_sys_rbuf_get(hio_sys_rbuf_t *rbuf, uint8_t *data, size_t bytes);

#ifdef __cplusplus
}
#endif

#endif
