#include <hio_rtc.h>

// Zephyr includes
#include <drivers/clock_control.h>
#include <drivers/clock_control/nrf_clock_control.h>
#include <irq.h>
#include <logging/log.h>
#include <nrfx_rtc.h>
#include <shell/shell.h>
#include <zephyr.h>

LOG_MODULE_REGISTER(hio_rtc, LOG_LEVEL_DBG);

static const nrfx_rtc_t rtc = NRFX_RTC_INSTANCE(0);
static struct onoff_client lfclk_cli;

static int year = 1970;
static int month = 1;
static int day = 1;
static int hours = 0;
static int minutes = 0;
static int seconds = 0;

static int
days_in_month(int year, int month)
{
    if (month < 1 || month > 12) {
        return 0;
    }

    if (month == 4 || month == 6 || month == 9 || month == 11) {
        return 30;
    } else if (month == 2) {
        if (year % 400 == 0 || (year % 100 != 0 && year % 4 == 0)) {
            return 29;
        } else {
            return 28;
        }
    }

    return 31;
}

static void
rtc_handler(nrfx_rtc_int_type_t int_type)
{
    if (int_type != NRFX_RTC_INT_TICK) {
        return;
	}

    static int prescaler = 0;

    if (++prescaler % 8 != 0) {
        return;
    }

    prescaler = 0;

    if (++seconds >= 60) {
        seconds = 0;
        if (++minutes >= 60) {
            minutes = 0;
            if (++hours >= 24) {
                hours = 0;
                if (++day > days_in_month(year, month)) {
                    day = 1;
                    if (++month > 12) {
                        month = 1;
                        ++year;
                    }
                }
            }
        }
    }
}

int
hio_rtc_init(void)
{
	struct onoff_manager *mgr =
		z_nrf_clock_control_get_onoff(CLOCK_CONTROL_NRF_SUBSYS_LF);

    if (mgr == NULL) {
        LOG_ERR("Call `z_nrf_clock_control_get_onoff` failed");
        return -1;
    }

    static struct k_poll_signal sig;

    k_poll_signal_init(&sig);
    sys_notify_init_signal(&lfclk_cli.notify, &sig);

	if (onoff_request(mgr, &lfclk_cli) < 0) {
        LOG_ERR("Call `onoff_request` failed");
        return -2;
    }

    struct k_poll_event events[] = {
        K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL,
                                 K_POLL_MODE_NOTIFY_ONLY,
                                 &sig)
    };

    if (k_poll(events, ARRAY_SIZE(events), K_FOREVER) < 0) {
        LOG_ERR("Call `k_poll` failed");
        return -3;
    }

    nrfx_rtc_config_t config = NRFX_RTC_DEFAULT_CONFIG;
    config.prescaler = 4095;

    if (nrfx_rtc_init(&rtc, &config, rtc_handler) != NRFX_SUCCESS) {
        LOG_ERR("Call `nrfx_rtc_init` failed");
        return -4;
    }

    nrfx_rtc_tick_enable(&rtc, true);
    nrfx_rtc_enable(&rtc);

    IRQ_CONNECT(RTC0_IRQn, 0, nrfx_rtc_0_irq_handler, NULL, 0);
    irq_enable(RTC0_IRQn);

    return 0;
}

static int
print_help(const struct shell *shell,
           size_t argc, char **argv)
{
	int ret = 0;

	if (argc > 1) {
		shell_error(shell, "%s: Command not found", argv[1]);
		ret = -1;
	}

	shell_help(shell);

	return ret;
}

static int
cmd_rtc_get(const struct shell *shell,
            size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    // shell_print(shell, "counter: %u", counter);

    return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
    sub_rtc,
    SHELL_CMD_ARG(get, NULL, "Get current date/time.",
                  cmd_rtc_get, 1, 0),
    SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(rtc, &sub_rtc, "RTC commands", print_help);
