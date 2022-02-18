#ifndef CHESTER_INCLUDE_BSP_H_
#define CHESTER_INCLUDE_BSP_H_

/* Zephyr includes */
#include <devicetree.h>

/* Standard includes */
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CTR_BSP_GP0A_DEV DT_LABEL(DT_NODELABEL(gpio0))
#define CTR_BSP_GP0A_PIN 3
#define CTR_BSP_GP1A_DEV DT_LABEL(DT_NODELABEL(gpio0))
#define CTR_BSP_GP1A_PIN 29
#define CTR_BSP_GP2A_DEV DT_LABEL(DT_NODELABEL(gpio0))
#define CTR_BSP_GP2A_PIN 2
#define CTR_BSP_GP3A_DEV DT_LABEL(DT_NODELABEL(gpio0))
#define CTR_BSP_GP3A_PIN 31

#define CTR_BSP_GP0B_DEV DT_LABEL(DT_NODELABEL(gpio0))
#define CTR_BSP_GP0B_PIN 28
#define CTR_BSP_GP1B_DEV DT_LABEL(DT_NODELABEL(gpio0))
#define CTR_BSP_GP1B_PIN 30
#define CTR_BSP_GP2B_DEV DT_LABEL(DT_NODELABEL(gpio0))
#define CTR_BSP_GP2B_PIN 4
#define CTR_BSP_GP3B_DEV DT_LABEL(DT_NODELABEL(gpio0))
#define CTR_BSP_GP3B_PIN 5

enum ctr_bsp_button {
	CTR_BSP_BUTTON_INT = 0,
	CTR_BSP_BUTTON_EXT = 1,
};

int ctr_bsp_get_button(enum ctr_bsp_button button, bool *pressed);
int ctr_bsp_set_w1b_slpz(int level);

#ifdef __cplusplus
}
#endif

#endif /* CHESTER_INCLUDE_BSP_H_ */
