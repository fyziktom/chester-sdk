#include <hio_net_lrw.h>
#include <hio_lrw_talk.h>
#include <hio_util.h>

// Zephyr includes
#include <logging/log.h>
#include <settings/settings.h>
#include <shell/shell.h>
#include <zephyr.h>

// Standard includes
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

LOG_MODULE_REGISTER(hio_net_lrw, LOG_LEVEL_DBG);

#define SETTINGS_PFX "lrw"

static K_MUTEX_DEFINE(m_settings_mut);

static bool m_enabled = false;

static bool m_adr = true;
static bool m_dutycycle = true;

enum band {
    BAND_AS923 = 0,
    BAND_AU915 = 1,
    BAND_EU868 = 5,
    BAND_KR920 = 6,
    BAND_IN865 = 7,
    BAND_US915 = 8
};

enum class {
    CLASS_A = 0,
    CLASS_C = 2
};

enum mode {
    MODE_ABP = 0,
    MODE_OTAA = 1
};

enum nwk {
    NWK_PRIVATE = 0,
    NWK_PUBLIC = 2
};

static enum band m_band = BAND_EU868;
static enum class m_class = CLASS_A;
static enum mode m_mode = MODE_OTAA;
static enum nwk m_nwk = NWK_PRIVATE;

static uint8_t m_devaddr[4];
static uint8_t m_deveui[8];
static uint8_t m_joineui[8];
static uint8_t m_appkey[16];
static uint8_t m_nwkskey[16];
static uint8_t m_appskey[16];

static int h_set(const char *key, size_t len,
                 settings_read_cb read_cb, void *cb_arg)
{
    int ret;
    const char *next;

    LOG_DBG("key: %s len: %u", key, len);

#define SETTINGS_SET(_key, _var)                             \
    do {                                                     \
        if (settings_name_steq(key, _key, &next) && !next) { \
            if (len != sizeof(_var)) {                       \
                return -EINVAL;                              \
            }                                                \
                                                             \
            k_mutex_lock(&m_settings_mut, K_FOREVER);        \
            ret = read_cb(cb_arg, &_var, len);               \
            k_mutex_unlock(&m_settings_mut);                 \
                                                             \
            if (ret < 0) {                                   \
                LOG_ERR("Call `read_cb` failed: %d", ret);   \
                return ret;                                  \
            }                                                \
                                                             \
            return 0;                                        \
        }                                                    \
    } while (0)

    SETTINGS_SET("enabled", m_enabled);
    SETTINGS_SET("band", m_band);
    SETTINGS_SET("class", m_class);
    SETTINGS_SET("mode", m_mode);
    SETTINGS_SET("nwk", m_nwk);
    SETTINGS_SET("adr", m_adr);
    SETTINGS_SET("dutycycle", m_dutycycle);
    SETTINGS_SET("devaddr", m_devaddr);
    SETTINGS_SET("deveui", m_deveui);
    SETTINGS_SET("joineui", m_joineui);
    SETTINGS_SET("appkey", m_appkey);
    SETTINGS_SET("nwkskey", m_nwkskey);
    SETTINGS_SET("appskey", m_appskey);

#undef SETTINGS_SET

    return -ENOENT;
}

static int h_export(int (*export_func)(const char *name,
			                           const void *val, size_t val_len))
{
    k_mutex_lock(&m_settings_mut, K_FOREVER);

#define EXPORT_FUNC(_key, _var, _size)                         \
    do {                                                       \
        (void)export_func(SETTINGS_PFX "/" _key, _var, _size); \
    } while (0)

    EXPORT_FUNC("band", &m_band, sizeof(m_band));
    EXPORT_FUNC("class", &m_class, sizeof(m_class));
    EXPORT_FUNC("mode", &m_mode, sizeof(m_mode));
    EXPORT_FUNC("nwk", &m_nwk, sizeof(m_nwk));
    EXPORT_FUNC("adr", &m_adr, sizeof(m_adr));
    EXPORT_FUNC("dutycycle", &m_dutycycle, sizeof(m_dutycycle));
    EXPORT_FUNC("devaddr", m_devaddr, sizeof(m_devaddr));
    EXPORT_FUNC("deveui", m_deveui, sizeof(m_deveui));
    EXPORT_FUNC("joineui", m_joineui, sizeof(m_joineui));
    EXPORT_FUNC("appkey", m_appkey, sizeof(m_appkey));
    EXPORT_FUNC("nwkskey", m_nwkskey, sizeof(m_nwkskey));
    EXPORT_FUNC("appskey", m_appskey, sizeof(m_appskey));

#undef EXPORT_FUNC

    k_mutex_unlock(&m_settings_mut);

    return 0;
}

static int configure(void)
{
    int ret;

    k_mutex_lock(&m_settings_mut, K_FOREVER);

    bool enabled = m_enabled;

    bool adr = m_adr;
    bool dutycycle = m_dutycycle;

    enum band band = m_band;
    enum class class = m_class;
    enum mode mode = m_mode;
    enum nwk nwk = m_nwk;

    uint8_t devaddr[sizeof(m_devaddr)];
    uint8_t deveui[sizeof(m_deveui)];
    uint8_t joineui[sizeof(m_joineui)];
    uint8_t appkey[sizeof(m_appkey)];
    uint8_t nwkskey[sizeof(m_nwkskey)];
    uint8_t appskey[sizeof(m_appskey)];

    memcpy(devaddr, m_devaddr, sizeof(devaddr));
    memcpy(deveui, m_deveui, sizeof(deveui));
    memcpy(joineui, m_joineui, sizeof(joineui));
    memcpy(appkey, m_appkey, sizeof(appkey));
    memcpy(nwkskey, m_nwkskey, sizeof(nwkskey));
    memcpy(appskey, m_appskey, sizeof(appskey));

    k_mutex_unlock(&m_settings_mut);

    ret = hio_lrw_talk_at_dformat(1);

    if (ret < 0) {
        LOG_ERR("Call `hio_lrw_talk_at_dformat` failed: %d", ret);
        return ret;
    }

    ret = hio_lrw_talk_at_band((uint8_t)band);

    if (ret < 0) {
        LOG_ERR("Call `hio_lrw_talk_at_band` failed: %d", ret);
        return ret;
    }

    ret = hio_lrw_talk_at_class((uint8_t)class);

    if (ret < 0) {
        LOG_ERR("Call `hio_lrw_talk_at_class` failed: %d", ret);
        return ret;
    }

    ret = hio_lrw_talk_at_mode((uint8_t)mode);

    if (ret < 0) {
        LOG_ERR("Call `hio_lrw_talk_at_mode` failed: %d", ret);
        return ret;
    }

    ret = hio_lrw_talk_at_nwk((uint8_t)nwk);

    if (ret < 0) {
        LOG_ERR("Call `hio_lrw_talk_at_nwk` failed: %d", ret);
        return ret;
    }

    ret = hio_lrw_talk_at_adr(adr ? 1 : 0);

    if (ret < 0) {
        LOG_ERR("Call `hio_lrw_talk_at_adr` failed: %d", ret);
        return ret;
    }

    ret = hio_lrw_talk_at_dutycycle(dutycycle ? 1 : 0);

    if (ret < 0) {
        LOG_ERR("Call `hio_lrw_talk_at_dutycycle` failed: %d", ret);
        return ret;
    }

    if (mode != MODE_ABP) {
        ret = hio_lrw_talk_at_joindc(dutycycle ? 1 : 0);

        if (ret < 0) {
            LOG_ERR("Call `hio_lrw_talk_at_joindc` failed: %d", ret);
            return ret;
        }
    }

    ret = hio_lrw_talk_at_devaddr(devaddr, sizeof(devaddr));

    if (ret < 0) {
        LOG_ERR("Call `hio_lrw_talk_at_devaddr` failed: %d", ret);
        return ret;
    }

    ret = hio_lrw_talk_at_deveui(deveui, sizeof(deveui));

    if (ret < 0) {
        LOG_ERR("Call `hio_lrw_talk_at_deveui` failed: %d", ret);
        return ret;
    }

    ret = hio_lrw_talk_at_appeui(joineui, sizeof(joineui));

    if (ret < 0) {
        LOG_ERR("Call `hio_lrw_talk_at_appeui` failed: %d", ret);
        return ret;
    }

    ret = hio_lrw_talk_at_appkey(appkey, sizeof(appkey));

    if (ret < 0) {
        LOG_ERR("Call `hio_lrw_talk_at_appkey` failed: %d", ret);
        return ret;
    }

    ret = hio_lrw_talk_at_nwkskey(nwkskey, sizeof(nwkskey));

    if (ret < 0) {
        LOG_ERR("Call `hio_lrw_talk_at_nwkskey` failed: %d", ret);
        return ret;
    }

    ret = hio_lrw_talk_at_appskey(appskey, sizeof(appskey));

    if (ret < 0) {
        LOG_ERR("Call `hio_lrw_talk_at_appskey` failed: %d", ret);
        return ret;
    }

    return 0;
}

int hio_net_lrw_init(void)
{
    int ret;

    LOG_INF("Start");

    static struct settings_handler sh = {
        .name = SETTINGS_PFX,
        .h_set = h_set,
        .h_export = h_export
    };

    ret = settings_register(&sh);

    if (ret < 0) {
        LOG_ERR("Call `settings_register` failed: %d", ret);
        return ret;
    }

    ret = settings_load_subtree(SETTINGS_PFX);

    if (ret < 0) {
        LOG_ERR("Call `settings_load_subtree` failed: %d", ret);
        return ret;
    }

    ret = hio_lrw_talk_init();

    if (ret < 0) {
        LOG_ERR("Call `hio_lrw_talk_init` failed: %d", ret);
        return ret;
    }

    ret = hio_lrw_talk_enable();

    if (ret < 0) {
        LOG_ERR("Call `hio_lrw_talk_enable` failed: %d", ret);
        return ret;
    }

    for (;;) {
        LOG_DBG("Alive");

        ret = hio_lrw_talk_at();

        if (ret < 0) {
            LOG_ERR("Call `hio_lrw_talk_at` failed: %d", ret);
            return ret;
        }

        ret = configure();

        if (ret < 0) {
            LOG_ERR("Call `configure` failed: %d", ret);
            return ret;
        }

        ret = hio_lrw_talk_at_join();

        if (ret < 0) {
            LOG_ERR("Call `hio_lrw_talk_at_join` failed: %d", ret);
            return ret;
        }

        k_sleep(K_SECONDS(30));
    }

    return 0;
}

static void print_enabled(const struct shell *shell)
{
    k_mutex_lock(&m_settings_mut, K_FOREVER);
    shell_print(shell, "enabled: %s", m_enabled ? "true" : "false");
    k_mutex_unlock(&m_settings_mut);
}

static void print_band(const struct shell *shell)
{
    k_mutex_lock(&m_settings_mut, K_FOREVER);

    switch (m_band) {
    case BAND_AS923:
        shell_print(shell, "band: as923");
        break;
    case BAND_AU915:
        shell_print(shell, "band: au915");
        break;
    case BAND_EU868:
        shell_print(shell, "band: eu868");
        break;
    case BAND_KR920:
        shell_print(shell, "band: kr920");
        break;
    case BAND_IN865:
        shell_print(shell, "band: in865");
        break;
    case BAND_US915:
        shell_print(shell, "band: us915");
        break;
    default:
        shell_print(shell, "band: (unknown)");
    }

    k_mutex_unlock(&m_settings_mut);
}

static void print_class(const struct shell *shell)
{
    k_mutex_lock(&m_settings_mut, K_FOREVER);

    switch (m_class) {
    case CLASS_A:
        shell_print(shell, "class: a");
        break;
    case CLASS_C:
        shell_print(shell, "class: c");
        break;
    default:
        shell_print(shell, "class: (unknown)");
    }

    k_mutex_unlock(&m_settings_mut);
}

static void print_mode(const struct shell *shell)
{
    k_mutex_lock(&m_settings_mut, K_FOREVER);

    switch (m_mode) {
    case MODE_ABP:
        shell_print(shell, "mode: abp");
        break;
    case MODE_OTAA:
        shell_print(shell, "mode: otaa");
        break;
    default:
        shell_print(shell, "mode: (unknown)");
    }

    k_mutex_unlock(&m_settings_mut);
}

static void print_nwk(const struct shell *shell)
{
    k_mutex_lock(&m_settings_mut, K_FOREVER);

    switch (m_nwk) {
    case NWK_PRIVATE:
        shell_print(shell, "nwk: private");
        break;
    case NWK_PUBLIC:
        shell_print(shell, "nwk: public");
        break;
    default:
        shell_print(shell, "nwk: (unknown)");
    }

    k_mutex_unlock(&m_settings_mut);
}

static void print_adr(const struct shell *shell)
{
    k_mutex_lock(&m_settings_mut, K_FOREVER);
    shell_print(shell, "adr: %s", m_adr ? "true" : "false");
    k_mutex_unlock(&m_settings_mut);
}

static void print_dutycycle(const struct shell *shell)
{
    k_mutex_lock(&m_settings_mut, K_FOREVER);
    shell_print(shell, "dutycycle: %s", m_dutycycle ? "true" : "false");
    k_mutex_unlock(&m_settings_mut);
}

static void print_devaddr(const struct shell *shell)
{
    k_mutex_lock(&m_settings_mut, K_FOREVER);

    char buf[sizeof(m_devaddr) * 2 + 1];

    if (hio_buf2hex(m_devaddr, sizeof(m_devaddr),
                    buf, sizeof(buf), false) >= 0) {
        shell_print(shell, "devaddr: %s", buf);
    }

    k_mutex_unlock(&m_settings_mut);
}

static void print_deveui(const struct shell *shell)
{
    k_mutex_lock(&m_settings_mut, K_FOREVER);

    char buf[sizeof(m_deveui) * 2 + 1];

    if (hio_buf2hex(m_deveui, sizeof(m_deveui),
                    buf, sizeof(buf), false) >= 0) {
        shell_print(shell, "deveui: %s", buf);
    }

    k_mutex_unlock(&m_settings_mut);
}

static void print_joineui(const struct shell *shell)
{
    k_mutex_lock(&m_settings_mut, K_FOREVER);

    char buf[sizeof(m_joineui) * 2 + 1];

    if (hio_buf2hex(m_joineui, sizeof(m_joineui),
                    buf, sizeof(buf), false) >= 0) {
        shell_print(shell, "joineui: %s", buf);
    }

    k_mutex_unlock(&m_settings_mut);
}

static void print_appkey(const struct shell *shell)
{
    k_mutex_lock(&m_settings_mut, K_FOREVER);

    char buf[sizeof(m_appkey) * 2 + 1];

    if (hio_buf2hex(m_appkey, sizeof(m_appkey),
                    buf, sizeof(buf), false) >= 0) {
        shell_print(shell, "appkey: %s", buf);
    }

    k_mutex_unlock(&m_settings_mut);
}

static void print_nwkskey(const struct shell *shell)
{
    k_mutex_lock(&m_settings_mut, K_FOREVER);

    char buf[sizeof(m_nwkskey) * 2 + 1];

    if (hio_buf2hex(m_nwkskey, sizeof(m_nwkskey),
                    buf, sizeof(buf), false) >= 0) {
        shell_print(shell, "nwkskey: %s", buf);
    }

    k_mutex_unlock(&m_settings_mut);
}

static void print_appskey(const struct shell *shell)
{
    k_mutex_lock(&m_settings_mut, K_FOREVER);

    char buf[sizeof(m_appskey) * 2 + 1];

    if (hio_buf2hex(m_appskey, sizeof(m_appskey),
                    buf, sizeof(buf), false) >= 0) {
        shell_print(shell, "appskey: %s", buf);
    }

    k_mutex_unlock(&m_settings_mut);
}

static int cmd_show(const struct shell *shell, size_t argc, char **argv)
{
    k_mutex_lock(&m_settings_mut, K_FOREVER);

    print_enabled(shell);
    print_band(shell);
    print_class(shell);
    print_mode(shell);
    print_nwk(shell);
    print_adr(shell);
    print_dutycycle(shell);
    print_devaddr(shell);
    print_deveui(shell);
    print_joineui(shell);
    print_appkey(shell);
    print_nwkskey(shell);
    print_appskey(shell);

    k_mutex_unlock(&m_settings_mut);

    return 0;
}

static int cmd_enabled(const struct shell *shell, size_t argc, char **argv)
{
    if (argc == 1) {
        print_enabled(shell);
        return 0;
    }

    if (argc == 2 && strcmp(argv[1], "true") == 0) {
        k_mutex_lock(&m_settings_mut, K_FOREVER);
        m_enabled = true;
        k_mutex_unlock(&m_settings_mut);
        return 0;
    }

    if (argc == 2 && strcmp(argv[1], "false") == 0) {
        k_mutex_lock(&m_settings_mut, K_FOREVER);
        m_enabled = false;
        k_mutex_unlock(&m_settings_mut);
        return 0;
    }

    shell_help(shell);
    return -EINVAL;
}

static int cmd_band(const struct shell *shell, size_t argc, char **argv)
{
    if (argc == 1) {
        print_band(shell);
        return 0;
    }

    if (argc == 2 && strcmp(argv[1], "as923") == 0) {
        k_mutex_lock(&m_settings_mut, K_FOREVER);
        m_band = BAND_AS923;
        k_mutex_unlock(&m_settings_mut);
        return 0;
    }

    if (argc == 2 && strcmp(argv[1], "au915") == 0) {
        k_mutex_lock(&m_settings_mut, K_FOREVER);
        m_band = BAND_AU915;
        k_mutex_unlock(&m_settings_mut);
        return 0;
    }

    if (argc == 2 && strcmp(argv[1], "eu868") == 0) {
        k_mutex_lock(&m_settings_mut, K_FOREVER);
        m_band = BAND_EU868;
        k_mutex_unlock(&m_settings_mut);
        return 0;
    }

    if (argc == 2 && strcmp(argv[1], "kr920") == 0) {
        k_mutex_lock(&m_settings_mut, K_FOREVER);
        m_band = BAND_KR920;
        k_mutex_unlock(&m_settings_mut);
        return 0;
    }

    if (argc == 2 && strcmp(argv[1], "in865") == 0) {
        k_mutex_lock(&m_settings_mut, K_FOREVER);
        m_band = BAND_IN865;
        k_mutex_unlock(&m_settings_mut);
        return 0;
    }

    if (argc == 2 && strcmp(argv[1], "us915") == 0) {
        k_mutex_lock(&m_settings_mut, K_FOREVER);
        m_band = BAND_US915;
        k_mutex_unlock(&m_settings_mut);
        return 0;
    }

    shell_help(shell);
    return -EINVAL;
}

static int cmd_class(const struct shell *shell, size_t argc, char **argv)
{
    if (argc == 1) {
        print_class(shell);
        return 0;
    }

    if (argc == 2 && strcmp(argv[1], "a") == 0) {
        k_mutex_lock(&m_settings_mut, K_FOREVER);
        m_class = CLASS_A;
        k_mutex_unlock(&m_settings_mut);
        return 0;
    }

    if (argc == 2 && strcmp(argv[1], "c") == 0) {
        k_mutex_lock(&m_settings_mut, K_FOREVER);
        m_class = CLASS_C;
        k_mutex_unlock(&m_settings_mut);
        return 0;
    }

    shell_help(shell);
    return -EINVAL;
}

static int cmd_mode(const struct shell *shell, size_t argc, char **argv)
{
    if (argc == 1) {
        print_mode(shell);
        return 0;
    }

    if (argc == 2 && strcmp(argv[1], "otaa") == 0) {
        k_mutex_lock(&m_settings_mut, K_FOREVER);
        m_mode = MODE_OTAA;
        k_mutex_unlock(&m_settings_mut);
        return 0;
    }

    if (argc == 2 && strcmp(argv[1], "abp") == 0) {
        k_mutex_lock(&m_settings_mut, K_FOREVER);
        m_mode = MODE_ABP;
        k_mutex_unlock(&m_settings_mut);
        return 0;
    }

    shell_help(shell);
    return -EINVAL;
}

static int cmd_nwk(const struct shell *shell, size_t argc, char **argv)
{
    if (argc == 1) {
        print_nwk(shell);
        return 0;
    }

    if (argc == 2 && strcmp(argv[1], "private") == 0) {
        k_mutex_lock(&m_settings_mut, K_FOREVER);
        m_nwk = NWK_PRIVATE;
        k_mutex_unlock(&m_settings_mut);
        return 0;
    }

    if (argc == 2 && strcmp(argv[1], "public") == 0) {
        k_mutex_lock(&m_settings_mut, K_FOREVER);
        m_nwk = NWK_PUBLIC;
        k_mutex_unlock(&m_settings_mut);
        return 0;
    }

    shell_help(shell);
    return -EINVAL;
}

static int cmd_adr(const struct shell *shell, size_t argc, char **argv)
{
    if (argc == 1) {
        print_adr(shell);
        return 0;
    }

    if (argc == 2 && strcmp(argv[1], "true") == 0) {
        k_mutex_lock(&m_settings_mut, K_FOREVER);
        m_adr = true;
        k_mutex_unlock(&m_settings_mut);
        return 0;
    }

    if (argc == 2 && strcmp(argv[1], "false") == 0) {
        k_mutex_lock(&m_settings_mut, K_FOREVER);
        m_adr = false;
        k_mutex_unlock(&m_settings_mut);
        return 0;
    }

    shell_help(shell);
    return -EINVAL;
}

static int cmd_dutycycle(const struct shell *shell, size_t argc, char **argv)
{
    if (argc == 1) {
        print_dutycycle(shell);
        return 0;
    }

    if (argc == 2 && strcmp(argv[1], "true") == 0) {
        k_mutex_lock(&m_settings_mut, K_FOREVER);
        m_dutycycle = true;
        k_mutex_unlock(&m_settings_mut);
        return 0;
    }

    if (argc == 2 && strcmp(argv[1], "false") == 0) {
        k_mutex_lock(&m_settings_mut, K_FOREVER);
        m_dutycycle = false;
        k_mutex_unlock(&m_settings_mut);
        return 0;
    }

    shell_help(shell);
    return -EINVAL;
}

static int cmd_devaddr(const struct shell *shell, size_t argc, char **argv)
{
    if (argc == 1) {
        print_devaddr(shell);
        return 0;
    }

    if (argc == 2) {
        k_mutex_lock(&m_settings_mut, K_FOREVER);
        int ret = hio_hex2buf(argv[1], m_devaddr, sizeof(m_devaddr), true);
        k_mutex_unlock(&m_settings_mut);

        if (ret == sizeof(m_devaddr)) {
            return 0;
        }
    }

    shell_help(shell);
    return -EINVAL;
}

static int cmd_deveui(const struct shell *shell, size_t argc, char **argv)
{
    if (argc == 1) {
        print_deveui(shell);
        return 0;
    }

    if (argc == 2) {
        k_mutex_lock(&m_settings_mut, K_FOREVER);
        int ret = hio_hex2buf(argv[1], m_deveui, sizeof(m_deveui), true);
        k_mutex_unlock(&m_settings_mut);

        if (ret == sizeof(m_deveui)) {
            return 0;
        }
    }

    shell_help(shell);
    return -EINVAL;
}

static int cmd_joineui(const struct shell *shell, size_t argc, char **argv)
{
    if (argc == 1) {
        print_joineui(shell);
        return 0;
    }

    if (argc == 2) {
        k_mutex_lock(&m_settings_mut, K_FOREVER);
        int ret = hio_hex2buf(argv[1], m_joineui, sizeof(m_joineui), true);
        k_mutex_unlock(&m_settings_mut);

        if (ret >= 0) {
            return 0;
        }
    }

    shell_help(shell);
    return -EINVAL;
}

static int cmd_appkey(const struct shell *shell, size_t argc, char **argv)
{
    if (argc == 1) {
        print_appkey(shell);
        return 0;
    }

    if (argc == 2) {
        k_mutex_lock(&m_settings_mut, K_FOREVER);
        int ret = hio_hex2buf(argv[1], m_appkey, sizeof(m_appkey), true);
        k_mutex_unlock(&m_settings_mut);

        if (ret >= 0) {
            return 0;
        }
    }

    shell_help(shell);
    return -EINVAL;
}

static int cmd_nwkskey(const struct shell *shell, size_t argc, char **argv)
{
    if (argc == 1) {
        print_nwkskey(shell);
        return 0;
    }

    if (argc == 2) {
        k_mutex_lock(&m_settings_mut, K_FOREVER);
        int ret = hio_hex2buf(argv[1], m_nwkskey, sizeof(m_nwkskey), true);
        k_mutex_unlock(&m_settings_mut);

        if (ret >= 0) {
            return 0;
        }
    }

    shell_help(shell);
    return -EINVAL;
}

static int cmd_appskey(const struct shell *shell, size_t argc, char **argv)
{
    if (argc == 1) {
        print_appskey(shell);
        return 0;
    }

    if (argc == 2) {
        k_mutex_lock(&m_settings_mut, K_FOREVER);
        int ret = hio_hex2buf(argv[1], m_appskey, sizeof(m_appskey), true);
        k_mutex_unlock(&m_settings_mut);

        if (ret >= 0) {
            return 0;
        }
    }

    shell_help(shell);
    return -EINVAL;
}

static int print_help(const struct shell *shell, size_t argc, char **argv)
{
    if (argc > 1) {
        shell_error(shell, "%s: Command not found", argv[1]);
        shell_help(shell);
        return -EINVAL;
    }

    shell_help(shell);

    return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
    sub_lrw,
    SHELL_CMD_ARG(show, NULL,
                  "List current configuration.",
                  cmd_show, 1, 0),
    SHELL_CMD_ARG(enabled, NULL,
                  "Get/Set LoRaWAN state (format: <true|false>).",
                  cmd_enabled, 1, 1),
    SHELL_CMD_ARG(band, NULL,
                  "Get/Set radio band "
                  "(format: <as923|au915|eu868|kr920|in865|us915>).",
                  cmd_band, 1, 1),
    SHELL_CMD_ARG(class, NULL,
                  "Get/Set device class (format: <a|c>).",
                  cmd_class, 1, 1),
    SHELL_CMD_ARG(mode, NULL,
                  "Get/Set operation mode (format: <abp|otaa>).",
                  cmd_mode, 1, 1),
    SHELL_CMD_ARG(nwk, NULL,
                  "Get/Set network type (format: <private|public>).",
                  cmd_nwk, 1, 1),
    SHELL_CMD_ARG(adr, NULL,
                  "Get/Set adaptive data rate (format: <true|false>).",
                  cmd_adr, 1, 1),
    SHELL_CMD_ARG(dutycycle, NULL,
                  "Get/Set duty cycle (format: <true|false>).",
                  cmd_dutycycle, 1, 1),
    SHELL_CMD_ARG(devaddr, NULL,
                  "Get/Set DevAddr (format: <8 hexadecimal digits>).",
                  cmd_devaddr, 1, 1),
    SHELL_CMD_ARG(deveui, NULL,
                  "Get/Set DevEUI (format: <16 hexadecimal digits>).",
                  cmd_deveui, 1, 1),
    SHELL_CMD_ARG(joineui, NULL,
                  "Get/Set JoinEUI (format: <16 hexadecimal digits>).",
                  cmd_joineui, 1, 1),
    SHELL_CMD_ARG(appkey, NULL,
                  "Get/Set AppKey (format: <32 hexadecimal digits>).",
                  cmd_appkey, 1, 1),
    SHELL_CMD_ARG(nwkskey, NULL,
                  "Get/Set NwkSKey (format: <32 hexadecimal digits>).",
                  cmd_nwkskey, 1, 1),
    SHELL_CMD_ARG(appskey, NULL,
                  "Get/Set AppSKey (format: <32 hexadecimal digits>).",
                  cmd_appskey, 1, 1),
    SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(lrw, &sub_lrw, "LoRaWAN commands", print_help);
