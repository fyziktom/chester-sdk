#include "app_init.h"
#include "app_data.h"
#include "app_handler.h"

/* CHESTER includes */
#include <ctr_led.h>
#include <ctr_lrw.h>
#include <ctr_lte.h>
#include <ctr_wdog.h>

/* Zephyr includes */
#include <device.h>
#include <devicetree.h>
#include <drivers/gpio.h>
#include <logging/log.h>
#include <sys/byteorder.h>
#include <zephyr.h>

/* Standard includes */
#include <stddef.h>
#include <stdint.h>

LOG_MODULE_REGISTER(app_init, LOG_LEVEL_DBG);

#if defined(CONFIG_SHIELD_CTR_LTE)
K_SEM_DEFINE(g_app_init_sem, 0, 1);
#endif /* defined(CONFIG_SHIELD_CTR_LTE) */

struct ctr_wdog_channel g_app_wdog_channel;

int app_init(void)
{
	int ret;

	ctr_led_set(CTR_LED_CHANNEL_R, true);

	ret = ctr_wdog_set_timeout(120000);
	if (ret) {
		LOG_ERR("Call `ctr_wdog_set_timeout` failed: %d", ret);
		return ret;
	}

	ret = ctr_wdog_install(&g_app_wdog_channel);
	if (ret) {
		LOG_ERR("Call `ctr_wdog_install` failed: %d", ret);
		return ret;
	}

	ret = ctr_wdog_start();
	if (ret) {
		LOG_ERR("Call `ctr_wdog_start` failed: %d", ret);
		return ret;
	}

#if defined(CONFIG_SHIELD_CTR_LRW)
	ret = ctr_lrw_init(app_handler_lrw, NULL);
	if (ret) {
		LOG_ERR("Call `ctr_lrw_init` failed: %d", ret);
		return ret;
	}

	ret = ctr_lrw_start(NULL);
	if (ret) {
		LOG_ERR("Call `ctr_lrw_start` failed: %d", ret);
		return ret;
	}

	k_sleep(K_SECONDS(2));
#endif /* defined(CONFIG_SHIELD_CTR_LRW) */

#if defined(CONFIG_SHIELD_CTR_LTE)
	ret = ctr_lte_set_event_cb(app_handler_lte, NULL);
	if (ret) {
		LOG_ERR("Call `ctr_lte_set_event_cb` failed: %d", ret);
		return ret;
	}

	ret = ctr_lte_start(NULL);
	if (ret) {
		LOG_ERR("Call `ctr_lte_start` failed: %d", ret);
		return ret;
	}

	for (;;) {
		ret = ctr_wdog_feed(&g_app_wdog_channel);
		if (ret) {
			LOG_ERR("Call `ctr_wdog_feed` failed: %d", ret);
			return ret;
		}

		ret = k_sem_take(&g_app_init_sem, K_SECONDS(1));
		if (ret == -EAGAIN) {
			continue;
		} else if (ret) {
			LOG_ERR("Call `k_sem_take` failed: %d", ret);
			return ret;
		}

		break;
	}
#endif /* defined(CONFIG_SHIELD_CTR_LTE) */

	ctr_led_set(CTR_LED_CHANNEL_R, false);

	return 0;
}