#include <ctr_bsp.h>
#include <ctr_rtc.h>

/* Zephyr includes */
#include <device.h>
#include <devicetree.h>
#include <drivers/gpio.h>
#include <drivers/spi.h>
#include <logging/log.h>
#include <zephyr.h>

LOG_MODULE_REGISTER(ctr_bsp, LOG_LEVEL_DBG);

#define DEV_BTN_INT m_dev_gpio_0
#define PIN_BTN_INT 27

#define DEV_BTN_EXT m_dev_gpio_0
#define PIN_BTN_EXT 26

#define DEV_SLPZ m_dev_gpio_1
#define PIN_SLPZ 10

static const struct device *m_dev_gpio_0;
static const struct device *m_dev_gpio_1;
static const struct device *m_dev_spi_1;

static int init_gpio(void)
{
	m_dev_gpio_0 = device_get_binding("GPIO_0");

	if (m_dev_gpio_0 == NULL) {
		LOG_ERR("Call `device_get_binding` failed");
		return -ENODEV;
	}

	m_dev_gpio_1 = device_get_binding("GPIO_1");

	if (m_dev_gpio_1 == NULL) {
		LOG_ERR("Call `device_get_binding` failed");
		return -ENODEV;
	}

	return 0;
}

static int init_button(void)
{
	int ret;

	ret = gpio_pin_configure(DEV_BTN_INT, PIN_BTN_INT, GPIO_INPUT | GPIO_PULL_UP);

	if (ret < 0) {
		LOG_ERR("Call `gpio_pin_configure` failed: %d", ret);
		return ret;
	}

	ret = gpio_pin_configure(DEV_BTN_EXT, PIN_BTN_EXT, GPIO_INPUT | GPIO_PULL_UP);

	if (ret < 0) {
		LOG_ERR("Call `gpio_pin_configure` failed: %d", ret);
		return ret;
	}

	return 0;
}

static int init_flash(void)
{
	int ret;

	m_dev_spi_1 = device_get_binding("SPI_1");

	if (m_dev_spi_1 == NULL) {
		LOG_ERR("Call `device_get_binding` failed");
		return -ENODEV;
	}

	struct spi_cs_control spi_cs = { 0 };

	spi_cs.gpio_dev = m_dev_gpio_0;
	spi_cs.gpio_pin = 23;
	spi_cs.gpio_dt_flags = GPIO_ACTIVE_LOW;
	spi_cs.delay = 2;

	struct spi_config spi_cfg = { 0 };

	spi_cfg.frequency = 500000;
	spi_cfg.operation = SPI_OP_MODE_MASTER | SPI_WORD_SET(8) | SPI_LINES_SINGLE;
	spi_cfg.cs = &spi_cs;

	uint8_t buffer_tx[] = { 0x79 };

	const struct spi_buf tx_buf[] = { { .buf = buffer_tx, .len = sizeof(buffer_tx) } };

	const struct spi_buf_set tx = { .buffers = tx_buf, .count = 1 };

	const struct spi_buf rx_buf[] = { { .buf = NULL, .len = 1 } };

	const struct spi_buf_set rx = { .buffers = rx_buf, .count = 1 };

	ret = spi_transceive(m_dev_spi_1, &spi_cfg, &tx, &rx);

	if (ret < 0) {
		LOG_ERR("Call `spi_transceive` failed");
		return ret;
	}

	return 0;
}

int init_w1b(void)
{
	int ret;

	ret = gpio_pin_configure(DEV_SLPZ, PIN_SLPZ, GPIO_OUTPUT_INACTIVE);

	if (ret < 0) {
		LOG_ERR("Call `gpio_pin_configure` failed: %d", ret);
		return ret;
	}

	return 0;
}

int ctr_bsp_get_button(enum ctr_bsp_button button, bool *pressed)
{
	int ret;

	switch (button) {
	case CTR_BSP_BUTTON_INT:
		ret = gpio_pin_get(DEV_BTN_INT, PIN_BTN_INT);

		if (ret < 0) {
			LOG_ERR("Call `gpio_pin_get` failed: %d", ret);
			return ret;
		}

		*pressed = ret == 0 ? true : false;
		break;

	case CTR_BSP_BUTTON_EXT:

		ret = gpio_pin_get(DEV_BTN_EXT, PIN_BTN_EXT);

		if (ret < 0) {
			LOG_ERR("Call `gpio_pin_get` failed: %d", ret);
			return ret;
		}

		*pressed = ret == 0 ? true : false;
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

int ctr_bsp_set_w1b_slpz(int level)
{
	int ret;

	ret = gpio_pin_set(DEV_SLPZ, PIN_SLPZ, level == 0 ? 0 : 1);

	if (ret < 0) {
		LOG_ERR("Call `gpio_pin_set` failed: %d", ret);
		return ret;
	}

	return 0;
}

static int init(const struct device *dev)
{
	ARG_UNUSED(dev);

	int ret;

	LOG_INF("System initialization");

	ret = init_gpio();

	if (ret < 0) {
		LOG_ERR("Call `init_gpio` failed: %d", ret);
		return ret;
	}

	ret = init_button();

	if (ret < 0) {
		LOG_ERR("Call `init_button` failed: %d", ret);
		return ret;
	}

	ret = init_flash();

	if (ret < 0) {
		LOG_ERR("Call `init_flash` failed: %d", ret);
		return ret;
	}

	ret = init_w1b();

	if (ret < 0) {
		LOG_ERR("Call `init_w1b` failed: %d", ret);
		return ret;
	}

	return 0;
}

SYS_INIT(init, APPLICATION, CONFIG_CTR_BSP_INIT_PRIORITY);
