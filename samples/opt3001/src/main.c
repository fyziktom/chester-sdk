/* Zephyr includes */
#include <device.h>
#include <devicetree.h>
#include <drivers/sensor.h>
#include <logging/log.h>
#include <zephyr.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

void main(void)
{
	int ret;

	LOG_INF("Build time: " __DATE__ " " __TIME__);

	static const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(opt3001));

	if (!device_is_ready(dev)) {
		LOG_ERR("Device not ready");
		k_oops();
	}

	for (;;) {
		k_sleep(K_SECONDS(1));

		ret = sensor_sample_fetch(dev);
		if (ret) {
			LOG_ERR("Call `sensor_sample_fetch` failed: %d", ret);
			continue;
		}

		struct sensor_value val;
		ret = sensor_channel_get(dev, SENSOR_CHAN_LIGHT, &val);
		if (ret) {
			LOG_ERR("Call `sensor_channel_get` failed: %d", ret);
			continue;
		}

		LOG_INF("Illuminance: %d lux", val.val1);
	}
}