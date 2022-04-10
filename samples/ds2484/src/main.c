/* Zephyr includes */
#include <device.h>
#include <devicetree.h>
#include <drivers/w1.h>
#include <logging/log.h>
#include <zephyr.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

void search_callback(struct w1_rom rom, void *cb_arg)
{
	char serial_buf[13] = { 0 };
	size_t serial_buf_len =
	        bin2hex(rom.serial, sizeof(rom.serial), serial_buf, sizeof(serial_buf));
	LOG_INF("Device found; family: 0x%x, serial: 0x%.*s", rom.family, serial_buf_len,
	        serial_buf);
}

void main(void)
{
	LOG_INF("Build time: " __DATE__ " " __TIME__);

	const struct device *dev_ds2484 = DEVICE_DT_GET(DT_NODELABEL(ds2484));

	w1_lock_bus(dev_ds2484);
	bool ppd = w1_reset_bus(dev_ds2484);
	w1_unlock_bus(dev_ds2484);

	if (ppd) {
		LOG_INF("At least one device present on bus");
	} else {
		LOG_INF("No devices present on bus");
	}

	for (;;) {
		w1_lock_bus(dev_ds2484);
		size_t num_devices = w1_search_bus(dev_ds2484, W1_CMD_SEARCH_ROM,
		                                   W1_SEARCH_ALL_FAMILIES, search_callback, NULL);
		w1_unlock_bus(dev_ds2484);

		LOG_INF("Number of devices found on bus: %u", num_devices);

		k_sleep(K_SECONDS(10));
	}
}