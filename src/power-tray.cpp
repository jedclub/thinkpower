#include <gtk/gtk.h>
#include <libayatana-appindicator/app-indicator.h>
#include <gio/gio.h>
#include <glib.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <cmath>
#include <iomanip>
#include <cstdlib>
#include <unistd.h>
#include <pwd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <dlfcn.h>
#include <csignal>
#include "l10n.hpp"

#if defined(__AVX2__)
#include <immintrin.h>
#define THINKPOWER_HAS_AVX2 1
#endif

static const char *APPINDICATOR_ID = "power_profile_indicator";

// Direct low-level syscall reader (avoids std::ifstream locale and virtual dispatch overhead)
static inline bool fast_read_sysfs(const std::string &path, char *out_buf, size_t max_len) {
    int fd = open(path.c_str(), O_RDONLY | O_CLOEXEC);
    if (fd < 0) return false;
    ssize_t n = read(fd, out_buf, max_len - 1);
    close(fd);
    if (n <= 0) return false;
    out_buf[n] = '\0';
    while (n > 0 && (out_buf[n - 1] == '\n' || out_buf[n - 1] == '\r' || out_buf[n - 1] == ' ' || out_buf[n - 1] == '\t')) {
        out_buf[--n] = '\0';
    }
    return true;
}

static inline long fast_parse_long_buf(const char *buf, long def_val = 0) {
    while (*buf == ' ' || *buf == '\t' || *buf == '\n') buf++;
    if (*buf == '\0') return def_val;
    bool neg = false;
    if (*buf == '-') { neg = true; buf++; }
    long val = 0;
    while (*buf >= '0' && *buf <= '9') {
        val = val * 10 + (*buf - '0');
        buf++;
    }
    return neg ? -val : val;
}

// 256-bit AVX2 SIMD Vectorized Moving Average Filter for Wattage
struct alignas(32) PowerFilterSIMD {
    static constexpr size_t SAMPLES = 8;
    double history[SAMPLES] = {0.0};
    size_t count = 0;
    size_t head = 0;

    void add_sample(double w) {
        history[head] = w;
        head = (head + 1) % SAMPLES;
        if (count < SAMPLES) count++;
    }

    double get_average() const {
        if (count == 0) return 0.0;
#if THINKPOWER_HAS_AVX2
        // 256-bit AVX2 SIMD reduction across 8 double-precision floats
        __m256d ymm0 = _mm256_load_pd(&history[0]);
        __m256d ymm1 = _mm256_load_pd(&history[4]);
        __m256d ymm_sum = _mm256_add_pd(ymm0, ymm1);

        __m128d hi = _mm256_extractf128_pd(ymm_sum, 1);
        __m128d lo = _mm256_castpd256_pd128(ymm_sum);
        __m128d sum128 = _mm_add_pd(lo, hi);
        __m128d high_pair = _mm_unpackhi_pd(sum128, sum128);
        __m128d final_sum = _mm_add_sd(sum128, high_pair);

        return _mm_cvtsd_f64(final_sum) / static_cast<double>(count);
#else
        double sum = 0.0;
        for (size_t i = 0; i < count; ++i) sum += history[i];
        return sum / static_cast<double>(count);
#endif
    }
};

struct PeripheralInfo {
    std::string name;
    std::string icon;
    int pct = 0;
};

struct BatteryInfo {
    int capacity = 0;
    std::string status = "Unknown";
    bool ac_online = false;
    int charge_start = 0;
    int charge_limit = 100;
    double power_w = 0.0;
    std::string time_str;
    int cycle_count = 0;
    double health_pct = 100.0;
    std::string protect_str;
    std::vector<PeripheralInfo> peripherals;
};

class PowerTrayApp {
public:
    PowerTrayApp();
    ~PowerTrayApp();

    void run();
    void update_state_ui();
    void switch_mode(const std::string &mode_key, const std::string &trigger_source = "user");
    void on_radio_toggled(GtkCheckMenuItem *item, const std::string &mode_key);
    void open_system_monitor();
    void on_restore_clicked();

    // D-Bus Signal Callback
    void on_dbus_signal(const std::string &new_os_profile);

    std::string get_manager_bin();
    std::string get_current_mode();

private:
    std::string get_cache_dir();
    std::string get_state_file_path();
    void set_current_mode(const std::string &mode);

    BatteryInfo get_battery_info();
    std::vector<PeripheralInfo> get_peripheral_batteries();
    void update_icon_label_and_tooltip(const std::string &mode, const BatteryInfo &bat);
    void build_menu();
    void notify_user(const std::string &title, const std::string &msg, const std::string &icon = "battery-profile-powersave-symbolic");
    void setup_dbus_listener();

    static gboolean timer_callback(gpointer user_data);
    static void radio_callback(GtkCheckMenuItem *item, gpointer user_data);
    static void monitor_callback(GtkMenuItem *item, gpointer user_data);
    static void restore_callback(GtkMenuItem *item, gpointer user_data);

    AppIndicator *indicator = nullptr;
    GtkWidget *menu = nullptr;
    GtkWidget *header_battery = nullptr;
    GtkWidget *header_power = nullptr;
    GtkWidget *header_protect = nullptr;
    GtkWidget *header_bt = nullptr;

    GSList *radio_group = nullptr;
    std::map<std::string, GtkWidget*> radio_items;

    bool updating_ui = false;
    std::string current_mode = "balanced";
    std::string previous_mode;
    gint64 last_switch_time_us = 0;
    gint64 user_manual_lock_until_us = 0;
    bool last_ac_online = false;
    bool is_first_run = true;
    gint64 last_ac_toggle_time_us = 0;
    std::string pending_dbus_expected_profile;

    GDBusConnection *dbus_conn = nullptr;
    guint dbus_sub_id = 0;
    PowerFilterSIMD power_filter;
};

struct RadioCallbackData {
    PowerTrayApp *app;
    std::string mode_key;
};

// ==============================================================================
// Implementation
// ==============================================================================

PowerTrayApp::PowerTrayApp() {
    current_mode = get_current_mode();
    if (current_mode.empty()) current_mode = "balanced";

    indicator = app_indicator_new(
        APPINDICATOR_ID,
        "battery-profile-balanced-symbolic",
        APP_INDICATOR_CATEGORY_HARDWARE
    );
    app_indicator_set_status(indicator, APP_INDICATOR_STATUS_ACTIVE);

    menu = gtk_menu_new();
    build_menu();
    app_indicator_set_menu(indicator, GTK_MENU(menu));

    setup_dbus_listener();
    update_state_ui();

    g_timeout_add_seconds(2, timer_callback, this);
}

PowerTrayApp::~PowerTrayApp() {
    if (get_current_mode() == "ultra") {
        std::string cmd = get_manager_bin() + " restore --internal >/dev/null 2>&1 &";
        system(cmd.c_str());
    }
    if (dbus_conn && dbus_sub_id > 0) {
        g_dbus_connection_signal_unsubscribe(dbus_conn, dbus_sub_id);
    }
    if (dbus_conn) {
        g_object_unref(dbus_conn);
    }
}

void PowerTrayApp::run() {
    gtk_main();
}

std::string PowerTrayApp::get_cache_dir() {
    const char *xdg = getenv("XDG_CACHE_HOME");
    if (xdg && xdg[0] != '\0') {
        return std::string(xdg);
    }
    const char *home = getenv("HOME");
    if (!home) {
        struct passwd *pw = getpwuid(getuid());
        if (pw) home = pw->pw_dir;
    }
    if (home) {
        return std::string(home) + "/.cache";
    }
    return "/tmp";
}

std::string PowerTrayApp::get_state_file_path() {
    return get_cache_dir() + "/power_profile_mode";
}

std::string PowerTrayApp::get_manager_bin() {
    std::string home_bin = getenv("HOME") ? (std::string(getenv("HOME")) + "/.local/bin/power-profile-manager") : "";
    if (!home_bin.empty() && access(home_bin.c_str(), X_OK) == 0) {
        return home_bin;
    }
    if (access("/usr/local/bin/power-profile-manager", X_OK) == 0) {
        return "/usr/local/bin/power-profile-manager";
    }
    char *path = g_find_program_in_path("power-profile-manager");
    if (path) {
        std::string res(path);
        g_free(path);
        return res;
    }
    return "power-profile-manager";
}

std::string PowerTrayApp::get_current_mode() {
    std::ifstream file(get_state_file_path());
    if (file.is_open()) {
        std::string mode;
        file >> mode;
        if (!mode.empty()) {
            current_mode = mode;
            return current_mode;
        }
    }
    if (!current_mode.empty()) return current_mode;
    return "balanced";
}

void PowerTrayApp::set_current_mode(const std::string &mode) {
    current_mode = mode;
    std::string dir = get_cache_dir();
    mkdir(dir.c_str(), 0755);
    std::string path = get_state_file_path();
    std::string tmp_path = path + ".tmp";
    std::ofstream file(tmp_path);
    if (file.is_open()) {
        file << mode << std::endl;
        file.close();
        chmod(tmp_path.c_str(), 0666);
        rename(tmp_path.c_str(), path.c_str());
    }
}

static std::string format_duration(double hours) {
    int h = static_cast<int>(hours);
    int m = static_cast<int>(std::round((hours - h) * 60.0));
    if (m >= 60) {
        h += 1;
        m = 0;
    }
    char buf[64];
    if (h > 0 && m > 0) {
        snprintf(buf, sizeof(buf), L10n::tr(L10n::StrId::TIME_HOURS_MINS_FMT), h, m);
    } else if (h > 0) {
        snprintf(buf, sizeof(buf), L10n::tr(L10n::StrId::TIME_HOURS_FMT), h);
    } else if (m > 0) {
        snprintf(buf, sizeof(buf), L10n::tr(L10n::StrId::TIME_MINS_FMT), m);
    } else {
        return L10n::tr(L10n::StrId::TIME_LESS_THAN_MIN);
    }
    return std::string(buf);
}

BatteryInfo PowerTrayApp::get_battery_info() {
    BatteryInfo info;
    std::string bat_dir = "/sys/class/power_supply/BAT0";
    if (access(bat_dir.c_str(), F_OK) != 0) {
        bat_dir = "/sys/class/power_supply/BAT1";
    }

    if (access(bat_dir.c_str(), F_OK) == 0) {
        char buf[64];
        auto read_long = [&](const std::string &file_name, long def_val = 0) -> long {
            if (fast_read_sysfs(bat_dir + "/" + file_name, buf, sizeof(buf))) {
                return fast_parse_long_buf(buf, def_val);
            }
            return def_val;
        };
        auto read_str = [&](const std::string &file_name, const std::string &def_val = "") -> std::string {
            if (fast_read_sysfs(bat_dir + "/" + file_name, buf, sizeof(buf))) {
                return std::string(buf);
            }
            return def_val;
        };

        info.capacity = static_cast<int>(read_long("capacity", 50));
        info.status = read_str("status", "Discharging");
        long power_now = read_long("power_now", 0);
        long voltage_now = read_long("voltage_now", 0);
        long current_now = read_long("current_now", 0);
        long energy_now = read_long("energy_now", 0);
        long energy_full = read_long("energy_full", 0);
        long energy_full_design = read_long("energy_full_design", 0);
        info.cycle_count = static_cast<int>(read_long("cycle_count", 0));

        long start_limit = read_long("charge_control_start_threshold", 0);
        if (start_limit <= 0) {
            start_limit = read_long("charge_start_threshold", 0);
        }
        info.charge_start = static_cast<int>(start_limit);

        long charge_limit = read_long("charge_control_end_threshold", 0);
        if (charge_limit <= 0) {
            charge_limit = read_long("charge_stop_threshold", 100);
        }
        if (charge_limit <= 0) charge_limit = 100;
        info.charge_limit = static_cast<int>(charge_limit);

        if (power_now <= 0 && voltage_now > 0 && current_now > 0) {
            power_now = (voltage_now / 1000) * (current_now / 1000);
        }

        if (energy_full_design > 0 && energy_full > 0) {
            info.health_pct = (static_cast<double>(energy_full) / energy_full_design) * 100.0;
        }

        if (power_now > 0) {
            info.power_w = static_cast<double>(power_now) / 1000000.0;
            power_filter.add_sample(info.power_w);
            double smooth_w = power_filter.get_average();
            double calc_power = (smooth_w > 0.0) ? (smooth_w * 1000000.0) : static_cast<double>(power_now);

            if (info.status == "Discharging" && energy_now > 0) {
                double hours = static_cast<double>(energy_now) / calc_power;
                info.time_str = format_duration(hours);
            } else if (info.status == "Charging") {
                double target_energy = energy_full * (info.charge_limit / 100.0);
                double diff = std::max(0.0, target_energy - energy_now);
                if (diff > 0) {
                    double hours = diff / calc_power;
                    std::string dur = format_duration(hours);
                    if (info.charge_limit < 100) {
                        char tbuf[128];
                        snprintf(tbuf, sizeof(tbuf), L10n::tr(L10n::StrId::TIME_TO_LIMIT_FMT), info.charge_limit);
                        info.time_str = std::string(tbuf) + dur;
                    } else {
                        info.time_str = std::string(L10n::tr(L10n::StrId::TIME_TO_FULL_PREFIX)) + dur;
                    }
                } else {
                    char tbuf[128];
                    snprintf(tbuf, sizeof(tbuf), L10n::tr(L10n::StrId::TIME_REACHING_SOON_FMT), info.charge_limit);
                    info.time_str = tbuf;
                }
            }
        }
    }

    char ac_buf[32];
    if (fast_read_sysfs("/sys/class/power_supply/AC/online", ac_buf, sizeof(ac_buf))) {
        info.ac_online = (ac_buf[0] == '1');
    } else if (fast_read_sysfs("/sys/class/power_supply/ACAD/online", ac_buf, sizeof(ac_buf))) {
        info.ac_online = (ac_buf[0] == '1');
    } else if (fast_read_sysfs("/sys/class/power_supply/ucsi-source-psy-USBC000:002/online", ac_buf, sizeof(ac_buf))) {
        info.ac_online = (ac_buf[0] == '1');
    }
    if (info.status == "Charging") {
        info.ac_online = true;
    }

    if (info.charge_limit < 100) {
        char pbuf[64];
        snprintf(pbuf, sizeof(pbuf), L10n::tr(L10n::StrId::PROTECT_LIMIT_FMT), info.charge_limit);
        info.protect_str = pbuf;
    } else {
        info.protect_str = L10n::tr(L10n::StrId::PROTECT_NORMAL);
    }

    info.peripherals = get_peripheral_batteries();
    return info;
}

std::vector<PeripheralInfo> PowerTrayApp::get_peripheral_batteries() {
    std::vector<PeripheralInfo> peripherals;
    if (!dbus_conn) return peripherals;

    GError *error = nullptr;
    GVariant *res = g_dbus_connection_call_sync(
        dbus_conn,
        "org.freedesktop.UPower",
        "/org/freedesktop/UPower",
        "org.freedesktop.UPower",
        "EnumerateDevices",
        nullptr,
        G_VARIANT_TYPE("(ao)"),
        G_DBUS_CALL_FLAGS_NONE,
        500,
        nullptr,
        &error
    );

    if (!res) {
        if (error) g_error_free(error);
        return peripherals;
    }

    GVariantIter *iter = nullptr;
    g_variant_get(res, "(ao)", &iter);
    const gchar *path = nullptr;

    while (g_variant_iter_loop(iter, "o", &path)) {
        GVariant *type_var = g_dbus_connection_call_sync(
            dbus_conn, "org.freedesktop.UPower", path, "org.freedesktop.DBus.Properties",
            "Get", g_variant_new("(ss)", "org.freedesktop.UPower.Device", "Type"),
            G_VARIANT_TYPE("(v)"), G_DBUS_CALL_FLAGS_NONE, 300, nullptr, nullptr
        );
        if (!type_var) continue;

        GVariant *v = nullptr;
        g_variant_get(type_var, "(v)", &v);
        guint32 dev_type = g_variant_get_uint32(v);
        g_variant_unref(v);
        g_variant_unref(type_var);

        // Type 2: Mouse, 3: Keyboard, 5: Headset, 6: Headphones, 7: Audio
        if (dev_type == 2 || dev_type == 3 || dev_type == 5 || dev_type == 6 || dev_type == 7) {
            GVariant *props = g_dbus_connection_call_sync(
                dbus_conn, "org.freedesktop.UPower", path, "org.freedesktop.DBus.Properties",
                "GetAll", g_variant_new("(s)", "org.freedesktop.UPower.Device"),
                G_VARIANT_TYPE("(a{sv})"), G_DBUS_CALL_FLAGS_NONE, 300, nullptr, nullptr
            );
            if (!props) continue;

            GVariantIter *piter = nullptr;
            g_variant_get(props, "(a{sv})", &piter);
            const gchar *k = nullptr;
            GVariant *val = nullptr;
            std::string model;
            double pct = 0.0;
            gboolean is_present = TRUE;

            while (g_variant_iter_loop(piter, "{&sv}", &k, &val)) {
                if (std::string(k) == "Model") {
                    model = g_variant_get_string(val, nullptr);
                } else if (std::string(k) == "Percentage") {
                    pct = g_variant_get_double(val);
                } else if (std::string(k) == "IsPresent") {
                    is_present = g_variant_get_boolean(val);
                }
            }
            g_variant_iter_free(piter);
            g_variant_unref(props);

            if (is_present && pct > 0.0) {
                std::string icon = "🎧";
                if (dev_type == 2) icon = "🖱️";
                else if (dev_type == 3) icon = "⌨️";

                if (model.empty()) {
                    model = (dev_type == 2 ? L10n::tr(L10n::StrId::DEV_MOUSE) :
                            (dev_type == 3 ? L10n::tr(L10n::StrId::DEV_KEYBOARD) :
                                             L10n::tr(L10n::StrId::DEV_AUDIO)));
                }
                peripherals.push_back({model, icon, static_cast<int>(std::round(pct))});
            }
        }
    }

    g_variant_iter_free(iter);
    g_variant_unref(res);
    return peripherals;
}

void PowerTrayApp::update_icon_label_and_tooltip(const std::string &mode, const BatteryInfo &bat) {
    int cap = bat.capacity;
    const std::string &status = bat.status;
    double power_w = bat.power_w;
    int charge_limit = bat.charge_limit;
    bool is_ac = bat.ac_online;

    // 1. Taskbar label (Charging +, Standby 🔌, Discharging -)
    std::ostringstream oss_lbl;
    if (status == "Charging") {
        oss_lbl << std::fixed << std::setprecision(1) << " ⚡ " << cap << "% (+" << power_w << "W)";
    } else if (status == "Full" || (status == "Not charging" && cap >= charge_limit)) {
        oss_lbl << " 🔌 " << cap << "% (" << L10n::tr(L10n::StrId::STATUS_COMPLETE_TAG) << ")";
    } else if (is_ac || status == "Not charging") {
        oss_lbl << " 🔌 " << cap << "% (" << L10n::tr(L10n::StrId::STATUS_STANDBY) << ")";
    } else {
        oss_lbl << std::fixed << std::setprecision(1) << " " << cap << "% (-" << power_w << "W)";
    }
    app_indicator_set_label(indicator, oss_lbl.str().c_str(), " ⚡ 100% (+00.0W)");

    // 2. Icon setup
    int pct_10 = std::min(100, std::max(0, static_cast<int>(std::round(cap / 10.0) * 10)));
    std::string p_name = "balanced";
    if (mode == "performance") p_name = "performance";
    else if (mode == "save" || mode == "ultra") p_name = "powersave";

    char icon_buf[128];
    if (status == "Charging" || ((is_ac || status == "Not charging") && status != "Discharging")) {
        snprintf(icon_buf, sizeof(icon_buf), "battery-%03d-charging-profile-%s", pct_10, p_name.c_str());
    } else {
        snprintf(icon_buf, sizeof(icon_buf), "battery-%03d-profile-%s", pct_10, p_name.c_str());
    }
    app_indicator_set_icon_full(indicator, icon_buf, (std::to_string(cap) + "% - " + mode).c_str());

    // 3. Tooltip setup (localized and distinct standby/full status)
    std::string status_txt;
    if (status == "Charging") {
        status_txt = L10n::tr(L10n::StrId::STATUS_CHARGING);
    } else if (status == "Full") {
        status_txt = L10n::tr(L10n::StrId::STATUS_FULL);
    } else if (status == "Not charging") {
        if (cap >= charge_limit) {
            char buf[128];
            snprintf(buf, sizeof(buf), L10n::tr(L10n::StrId::STATUS_CHARGE_LIMIT_FMT), charge_limit);
            status_txt = buf;
        } else if (bat.charge_start > 0 && cap >= bat.charge_start) {
            char buf[128];
            snprintf(buf, sizeof(buf), L10n::tr(L10n::StrId::STATUS_PENDING_RESUME_FMT), bat.charge_start);
            status_txt = buf;
        } else {
            status_txt = L10n::tr(L10n::StrId::STATUS_PENDING_AC);
        }
    } else if (status == "Discharging") {
        status_txt = L10n::tr(L10n::StrId::STATUS_DISCHARGING);
    } else {
        status_txt = is_ac ? L10n::tr(L10n::StrId::STATUS_PENDING_AC) : L10n::tr(L10n::StrId::STATUS_DISCHARGING);
    }
    std::string tooltip_title = std::string(L10n::tr(L10n::StrId::MENU_BATTERY_PREFIX)) + std::to_string(cap) + "% (" + status_txt + ")";

    std::map<std::string, std::string> mode_titles = {
        {"performance", L10n::tr(L10n::StrId::MODE_TITLE_PERF)},
        {"balanced",    L10n::tr(L10n::StrId::MODE_TITLE_BALANCED)},
        {"save",        L10n::tr(L10n::StrId::MODE_TITLE_SAVE)},
        {"ultra",       L10n::tr(L10n::StrId::MODE_TITLE_ULTRA)}
    };
    std::string mode_title = mode_titles.count(mode) ? mode_titles[mode] : mode;

    std::string power_line;
    std::string time_line;

    char pwr_buf[128];
    if (status == "Charging") {
        snprintf(pwr_buf, sizeof(pwr_buf), L10n::tr(L10n::StrId::POWER_CHARGE_RATE_FMT), power_w);
        power_line = pwr_buf;
        if (!bat.time_str.empty()) {
            time_line = "⏳ " + bat.time_str;
        }
    } else if (status == "Full" || (status == "Not charging" && cap >= charge_limit)) {
        snprintf(pwr_buf, sizeof(pwr_buf), L10n::tr(L10n::StrId::POWER_AC_DIRECT_FMT), charge_limit);
        power_line = pwr_buf;
    } else if (is_ac || status == "Not charging") {
        power_line = L10n::tr(L10n::StrId::POWER_AC_WAITING);
    } else {
        snprintf(pwr_buf, sizeof(pwr_buf), L10n::tr(L10n::StrId::POWER_DISCHARGE_RATE_FMT), power_w);
        power_line = pwr_buf;
        if (!bat.time_str.empty()) {
            time_line = std::string(L10n::tr(L10n::StrId::TIME_REMAINING_PREFIX)) + bat.time_str;
        }
    }

    std::string protect_line = std::string(L10n::tr(L10n::StrId::PROTECT_TITLE)) + bat.protect_str;

    char health_buf[128];
    snprintf(health_buf, sizeof(health_buf), L10n::tr(L10n::StrId::HEALTH_LINE_FMT), bat.health_pct, bat.cycle_count);
    std::string health_line = health_buf;

    std::vector<std::string> body_elements;
    body_elements.push_back(power_line);
    if (!time_line.empty()) {
        body_elements.push_back(time_line);
    }
    body_elements.push_back("⚙️ " + mode_title);
    body_elements.push_back(protect_line);
    body_elements.push_back(health_line);

    for (const auto &p : bat.peripherals) {
        body_elements.push_back(p.icon + " " + p.name + " : " + std::to_string(p.pct) + "%");
    }
    body_elements.push_back(L10n::tr(L10n::StrId::WIRELESS_STATUS));

    std::string tooltip_body;
    for (size_t i = 0; i < body_elements.size(); ++i) {
        tooltip_body += body_elements[i];
        if (i + 1 < body_elements.size()) tooltip_body += "\n";
    }

    typedef void (*set_tooltip_full_fn)(AppIndicator*, const gchar*, const gchar*, const gchar*);
    static set_tooltip_full_fn p_set_tooltip_full = (set_tooltip_full_fn)dlsym(RTLD_DEFAULT, "app_indicator_set_tooltip_full");
    if (p_set_tooltip_full) {
        p_set_tooltip_full(indicator, icon_buf, tooltip_title.c_str(), tooltip_body.c_str());
    } else {
        std::string full_title = tooltip_title + "\n" + tooltip_body;
        app_indicator_set_title(indicator, full_title.c_str());
    }
}

void PowerTrayApp::update_state_ui() {
    updating_ui = true;
    std::string mode = get_current_mode();
    BatteryInfo bat = get_battery_info();

    gint64 now_mono = g_get_monotonic_time();
    bool ac_connected = bat.ac_online || (bat.status == "Charging");
    bool ac_just_connected = (!last_ac_online && ac_connected);

    if (bat.ac_online != last_ac_online) {
        last_ac_online = bat.ac_online;
        last_ac_toggle_time_us = now_mono;
    }

    // [충전 모드 트리거 자동 복구]
    // 전원 공급/충전 상태로 들어갔을 때, 현재 모드가 초절전(ultra) 또는 절전(save)이면
    // 안전을 위해 강제로 균형(balanced) 모드로 자동 복구 전환하여 시스템 멈춤/오류를 원천 방지합니다.
    if ((ac_just_connected || (is_first_run && ac_connected)) && (mode == "ultra" || mode == "save")) {
        user_manual_lock_until_us = 0;
        switch_mode("balanced", "ac_trigger");
        mode = "balanced";
    }
    is_first_run = false;

    if (radio_items.count(mode) && radio_items[mode]) {
        GtkCheckMenuItem *chk = GTK_CHECK_MENU_ITEM(radio_items[mode]);
        if (!gtk_check_menu_item_get_active(chk)) {
            gtk_check_menu_item_set_active(chk, TRUE);
        }
    }

    update_icon_label_and_tooltip(mode, bat);

    std::string status_txt;
    if (bat.status == "Charging") {
        status_txt = L10n::tr(L10n::StrId::STATUS_CHARGING);
    } else if (bat.status == "Full") {
        status_txt = L10n::tr(L10n::StrId::STATUS_FULL);
    } else if (bat.status == "Not charging") {
        if (bat.capacity >= bat.charge_limit) {
            char buf[128];
            snprintf(buf, sizeof(buf), L10n::tr(L10n::StrId::STATUS_CHARGE_LIMIT_FMT), bat.charge_limit);
            status_txt = buf;
        } else if (bat.charge_start > 0 && bat.capacity >= bat.charge_start) {
            char buf[128];
            snprintf(buf, sizeof(buf), L10n::tr(L10n::StrId::STATUS_PENDING_RESUME_FMT), bat.charge_start);
            status_txt = buf;
        } else {
            status_txt = L10n::tr(L10n::StrId::STATUS_PENDING_AC);
        }
    } else if (bat.status == "Discharging") {
        status_txt = L10n::tr(L10n::StrId::STATUS_DISCHARGING);
    } else {
        status_txt = bat.ac_online ? L10n::tr(L10n::StrId::STATUS_PENDING_AC) : L10n::tr(L10n::StrId::STATUS_DISCHARGING);
    }

    std::string time_txt = bat.time_str.empty() ? "" : (" (" + bat.time_str + ")");
    std::string bat_lbl = std::string(L10n::tr(L10n::StrId::MENU_BATTERY_PREFIX)) + std::to_string(bat.capacity) + "% - " + status_txt + time_txt;
    gtk_menu_item_set_label(GTK_MENU_ITEM(header_battery), bat_lbl.c_str());

    char oss_pwr[128];
    if (bat.status == "Charging") {
        snprintf(oss_pwr, sizeof(oss_pwr), L10n::tr(L10n::StrId::MENU_CURRENT_CHARGE_FMT), bat.power_w);
    } else if (bat.status == "Full" || (bat.status == "Not charging" && bat.capacity >= bat.charge_limit)) {
        snprintf(oss_pwr, sizeof(oss_pwr), L10n::tr(L10n::StrId::MENU_AC_MAINTAINING_FMT), bat.charge_limit);
    } else if (bat.ac_online || bat.status == "Not charging") {
        snprintf(oss_pwr, sizeof(oss_pwr), "%s", L10n::tr(L10n::StrId::MENU_AC_WAITING));
    } else {
        snprintf(oss_pwr, sizeof(oss_pwr), L10n::tr(L10n::StrId::MENU_CURRENT_DISCHARGE_FMT), bat.power_w);
    }
    gtk_menu_item_set_label(GTK_MENU_ITEM(header_power), oss_pwr);

    std::string prot_lbl = std::string(L10n::tr(L10n::StrId::PROTECT_TITLE)) + bat.protect_str;
    gtk_menu_item_set_label(GTK_MENU_ITEM(header_protect), prot_lbl.c_str());

    if (!bat.peripherals.empty()) {
        std::string bt_str;
        for (size_t i = 0; i < bat.peripherals.size(); ++i) {
            bt_str += bat.peripherals[i].icon + " " + bat.peripherals[i].name + ": " + std::to_string(bat.peripherals[i].pct) + "%";
            if (i + 1 < bat.peripherals.size()) bt_str += " · ";
        }
        gtk_menu_item_set_label(GTK_MENU_ITEM(header_bt), bt_str.c_str());
    } else {
        gtk_menu_item_set_label(GTK_MENU_ITEM(header_bt), L10n::tr(L10n::StrId::DEV_NONE));
    }

    updating_ui = false;
}

void PowerTrayApp::switch_mode(const std::string &mode_key, const std::string &trigger_source) {
    if (mode_key == current_mode) {
        return;
    }

    gint64 now = g_get_monotonic_time();

    if (trigger_source == "dbus") {
        // 1. User manual selection lock (60 seconds)
        if (now < user_manual_lock_until_us) {
            return;
        }
        // 2. Hardware AC flap suppression (if AC toggled within 10s, ignore transient profile requests)
        if (now - last_ac_toggle_time_us < 10000000LL) {
            return;
        }
        // 3. Minimum D-Bus switch cooldown (10.0 seconds)
        if (now - last_switch_time_us < 10000000LL) {
            return;
        }
        // 4. Ping-pong oscillation guard (30.0 seconds before allowing reversal back to previous_mode)
        if (mode_key == previous_mode && (now - last_switch_time_us < 30000000LL)) {
            return;
        }
    } else if (trigger_source == "user") {
        // User explicitly picked a mode: protect from external D-Bus/AC overrides for 60 seconds
        user_manual_lock_until_us = now + (60LL * 1000000LL);
    } else if (trigger_source == "ac_trigger") {
        // Force recovery on AC power connection: reset manual lock and allow instant switch
        user_manual_lock_until_us = 0;
    }

    previous_mode = current_mode;
    last_switch_time_us = now;

    set_current_mode(mode_key);
    update_state_ui();

    if (trigger_source == "user") {
        if (mode_key == "ultra") {
            notify_user(
                L10n::tr(L10n::StrId::NOTIFY_ULTRA_TITLE),
                L10n::tr(L10n::StrId::NOTIFY_ULTRA_MSG),
                "battery-low"
            );
        } else if (mode_key == "save") {
            notify_user(
                L10n::tr(L10n::StrId::NOTIFY_SAVE_TITLE),
                L10n::tr(L10n::StrId::NOTIFY_SAVE_MSG),
                "battery-profile-powersave-symbolic"
            );
        } else if (mode_key == "balanced") {
            notify_user(
                L10n::tr(L10n::StrId::NOTIFY_BAL_TITLE),
                L10n::tr(L10n::StrId::NOTIFY_BAL_MSG),
                "battery-profile-balanced-symbolic"
            );
        } else if (mode_key == "performance") {
            notify_user(
                L10n::tr(L10n::StrId::NOTIFY_PERF_TITLE),
                L10n::tr(L10n::StrId::NOTIFY_PERF_MSG),
                "battery-profile-performance-symbolic"
            );
        }
    } else if (trigger_source == "ac_trigger") {
        notify_user(
            L10n::is_korean() ? "전원 공급 감지: [균형 모드 자동 전환]" : "AC Power Connected: [Balanced Mode Restored]",
            L10n::is_korean() ? "충전기가 연결되어 절전 모드를 해제하고 균형 모드로 자동 복구되었습니다." : "AC charger connected. Automatically restored to Balanced mode.",
            "battery-profile-balanced-symbolic"
        );
    }

    std::string os_target = "balanced";
    if (mode_key == "performance") os_target = "performance";
    else if (mode_key == "save" || mode_key == "ultra") os_target = "power-saver";

    // Only synchronize to PPD D-Bus when user-initiated from tray menu to prevent echo loops
    if (trigger_source != "dbus") {
        pending_dbus_expected_profile = os_target;
        std::string cmd_ppd = "powerprofilesctl set " + os_target;
        g_spawn_command_line_async(cmd_ppd.c_str(), nullptr);
    }

    // Call hardware manager with --internal to prevent it from issuing duplicate PPD calls and notifications
    std::string cmd_mgr = "sudo -n " + get_manager_bin() + " " + mode_key + " --internal";
    g_spawn_command_line_async(cmd_mgr.c_str(), nullptr);
}

void PowerTrayApp::on_radio_toggled(GtkCheckMenuItem *item, const std::string &mode_key) {
    if (updating_ui) return;
    if (gtk_check_menu_item_get_active(item)) {
        if (mode_key != current_mode) {
            switch_mode(mode_key, "user");
        }
    }
}

void PowerTrayApp::open_system_monitor() {
    if (g_find_program_in_path("plasma-systemmonitor")) {
        g_spawn_command_line_async("plasma-systemmonitor", nullptr);
    } else if (g_find_program_in_path("ksysguard")) {
        g_spawn_command_line_async("ksysguard", nullptr);
    } else {
        g_spawn_command_line_async("kde-open5 /usr/share/applications/org.kde.plasma-systemmonitor.desktop", nullptr);
    }
}

void PowerTrayApp::notify_user(const std::string &title, const std::string &msg, const std::string &icon) {
    char *argv[] = {
        (char*)"notify-send",
        (char*)"-a", (char*)"ThinkPower",
        (char*)"-h", (char*)"string:x-canonical-private-synchronous:thinkpower",
        (char*)"-t", (char*)"3000",
        (char*)"-i", (char*)icon.c_str(),
        (char*)title.c_str(),
        (char*)msg.c_str(),
        nullptr
    };
    g_spawn_async(nullptr, argv, nullptr, G_SPAWN_SEARCH_PATH, nullptr, nullptr, nullptr, nullptr);
}

// ==============================================================================
// D-Bus Signal Listener (Bidirectional Sync)
// ==============================================================================

static void on_dbus_signal_static(
    GDBusConnection *conn,
    const gchar *sender_name,
    const gchar *object_path,
    const gchar *interface_name,
    const gchar *signal_name,
    GVariant *parameters,
    gpointer user_data)
{
    (void)conn;
    (void)sender_name;
    (void)object_path;
    (void)interface_name;
    (void)signal_name;

    PowerTrayApp *app = static_cast<PowerTrayApp*>(user_data);
    if (!parameters || !app) return;

    const gchar *iface = nullptr;
    GVariantIter *changed_props = nullptr;
    g_variant_get(parameters, "(&sa{sv}as)", &iface, &changed_props, nullptr);

    if (std::string(iface) == "net.hadess.PowerProfiles") {
        const gchar *key = nullptr;
        GVariant *val = nullptr;
        while (g_variant_iter_loop(changed_props, "{&sv}", &key, &val)) {
            if (std::string(key) == "ActiveProfile") {
                const gchar *new_prof = g_variant_get_string(val, nullptr);
                app->on_dbus_signal(new_prof ? new_prof : "");
            }
        }
    }
    if (changed_props) g_variant_iter_free(changed_props);
}

void PowerTrayApp::on_dbus_signal(const std::string &new_os_profile) {
    if (new_os_profile.empty()) return;

    // Suppress echo if this signal matches our own recent user-triggered powerprofilesctl command
    if (!pending_dbus_expected_profile.empty() && new_os_profile == pending_dbus_expected_profile) {
        pending_dbus_expected_profile.clear();
        return;
    }

    // Refresh current mode from state file in case CLI (power-profile-manager) altered it
    current_mode = get_current_mode();

    // If current mode already aligns with the requested OS profile, do nothing
    if (new_os_profile == "performance" && current_mode == "performance") return;
    if (new_os_profile == "balanced" && current_mode == "balanced") return;
    if (new_os_profile == "power-saver" && (current_mode == "save" || current_mode == "ultra")) return;

    gint64 now = g_get_monotonic_time();

    // 1. User manual selection lock (60 seconds)
    if (now < user_manual_lock_until_us) {
        return;
    }

    // 2. Hardware AC flap suppression (if AC toggled within 10s, ignore transient profile requests)
    if (now - last_ac_toggle_time_us < 10000000LL) {
        return;
    }

    // 3. Minimum D-Bus switch cooldown (10.0 seconds)
    if (now - last_switch_time_us < 10000000LL) {
        return;
    }

    std::string target_mode = current_mode;
    if (new_os_profile == "performance") {
        target_mode = "performance";
    } else if (new_os_profile == "balanced") {
        target_mode = "balanced";
    } else if (new_os_profile == "power-saver") {
        if (current_mode != "ultra") {
            target_mode = "save";
        }
    }

    // 4. Ping-pong oscillation guard (30.0 seconds before allowing reversal back to previous_mode)
    if (target_mode == previous_mode && (now - last_switch_time_us < 30000000LL)) {
        return;
    }

    if (target_mode != current_mode) {
        switch_mode(target_mode, "dbus");
    }
}

void PowerTrayApp::setup_dbus_listener() {
    GError *error = nullptr;
    dbus_conn = g_bus_get_sync(G_BUS_TYPE_SYSTEM, nullptr, &error);
    if (!dbus_conn) {
        if (error) g_error_free(error);
        return;
    }

    dbus_sub_id = g_dbus_connection_signal_subscribe(
        dbus_conn,
        "net.hadess.PowerProfiles",
        "org.freedesktop.DBus.Properties",
        "PropertiesChanged",
        "/net/hadess/PowerProfiles",
        "net.hadess.PowerProfiles",
        G_DBUS_SIGNAL_FLAGS_NONE,
        on_dbus_signal_static,
        this,
        nullptr
    );
}

// ==============================================================================
// Menu Construction & Callbacks
// ==============================================================================

void PowerTrayApp::build_menu() {
    header_battery = gtk_menu_item_new_with_label(L10n::tr(L10n::StrId::MENU_BATTERY_PREFIX));
    gtk_widget_set_sensitive(header_battery, FALSE);
    gtk_widget_show(header_battery);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), header_battery);

    header_power = gtk_menu_item_new_with_label(L10n::tr(L10n::StrId::POWER_AC_WAITING));
    gtk_widget_set_sensitive(header_power, FALSE);
    gtk_widget_show(header_power);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), header_power);

    header_protect = gtk_menu_item_new_with_label(L10n::tr(L10n::StrId::PROTECT_TITLE));
    gtk_widget_set_sensitive(header_protect, FALSE);
    gtk_widget_show(header_protect);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), header_protect);

    header_bt = gtk_menu_item_new_with_label(L10n::tr(L10n::StrId::DEV_NONE));
    gtk_widget_set_sensitive(header_bt, FALSE);
    gtk_widget_show(header_bt);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), header_bt);

    GtkWidget *sep1 = gtk_separator_menu_item_new();
    gtk_widget_show(sep1);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), sep1);

    std::vector<std::pair<std::string, std::string>> modes = {
        {"performance", L10n::tr(L10n::StrId::MODE_MENU_PERF)},
        {"balanced",    L10n::tr(L10n::StrId::MODE_MENU_BALANCED)},
        {"save",        L10n::tr(L10n::StrId::MODE_MENU_SAVE)},
        {"ultra",       L10n::tr(L10n::StrId::MODE_MENU_ULTRA)}
    };

    for (const auto &m : modes) {
        GtkWidget *item = gtk_radio_menu_item_new_with_label(radio_group, m.second.c_str());
        radio_group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(item));

        RadioCallbackData *cb_data = new RadioCallbackData{this, m.first};
        g_signal_connect(item, "toggled", G_CALLBACK(radio_callback), cb_data);

        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
        gtk_widget_show(item);
        radio_items[m.first] = item;
    }

    GtkWidget *sep2 = gtk_separator_menu_item_new();
    gtk_widget_show(sep2);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), sep2);

    GtkWidget *monitor_item = gtk_menu_item_new_with_label(L10n::tr(L10n::StrId::MENU_SYSTEM_MONITOR));
    g_signal_connect(monitor_item, "activate", G_CALLBACK(monitor_callback), this);
    gtk_widget_show(monitor_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), monitor_item);

    GtkWidget *restore_item = gtk_menu_item_new_with_label(
        L10n::is_korean() ? "🛡️ 안전 복구 (기본값 원복)" : "🛡️ Failsafe Reset (Restore Defaults)"
    );
    g_signal_connect(restore_item, "activate", G_CALLBACK(restore_callback), this);
    gtk_widget_show(restore_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), restore_item);
}

gboolean PowerTrayApp::timer_callback(gpointer user_data) {
    PowerTrayApp *app = static_cast<PowerTrayApp*>(user_data);
    if (app) {
        app->update_state_ui();
    }
    return TRUE;
}

void PowerTrayApp::radio_callback(GtkCheckMenuItem *item, gpointer user_data) {
    RadioCallbackData *data = static_cast<RadioCallbackData*>(user_data);
    if (data && data->app) {
        data->app->on_radio_toggled(item, data->mode_key);
    }
}

void PowerTrayApp::monitor_callback(GtkMenuItem *item, gpointer user_data) {
    (void)item;
    PowerTrayApp *app = static_cast<PowerTrayApp*>(user_data);
    if (app) {
        app->open_system_monitor();
    }
}

void PowerTrayApp::restore_callback(GtkMenuItem *item, gpointer user_data) {
    (void)item;
    PowerTrayApp *app = static_cast<PowerTrayApp*>(user_data);
    if (app) {
        app->on_restore_clicked();
    }
}

void PowerTrayApp::on_restore_clicked() {
    switch_mode("balanced", "user");
    std::string cmd_mgr = "sudo -n " + get_manager_bin() + " restore --internal";
    g_spawn_command_line_async(cmd_mgr.c_str(), nullptr);
    notify_user(
        L10n::is_korean() ? "전원 프로파일: [안전 복구 완료]" : "Power Profile: [Failsafe Restored]",
        L10n::is_korean() ? "모든 CPU 코어(16스레드), 화면 주사율(60Hz), 데스크톱 효과가 기본값으로 복구되었습니다." : "All CPU cores, refresh rates, and desktop effects restored to default.",
        "security-high"
    );
}

// ==============================================================================
// PGO (Profile-Guided Optimization) Training Benchmark
// ==============================================================================

static void run_pgo_training(size_t iterations) {
    PowerFilterSIMD filter;
    std::cout << "[PGO] Running training workload (" << iterations << " iterations with AVX2 SIMD)..." << std::endl;
    for (size_t i = 0; i < iterations; ++i) {
        char buf[64];
        long cap = 75;
        if (fast_read_sysfs("/sys/class/power_supply/BAT0/capacity", buf, sizeof(buf))) {
            cap = fast_parse_long_buf(buf, 75);
        }
        (void)cap;

        double sample = 14.5 + (i % 8) * 0.7;
        filter.add_sample(sample);
        double avg = filter.get_average();
        (void)avg;

        std::string dur = format_duration(1.2 + (i % 6) * 0.3);
        (void)dur;

        const char *str_sample = L10n::tr(static_cast<L10n::StrId>(i % static_cast<size_t>(L10n::StrId::COUNT)));
        (void)str_sample;
    }
    std::cout << "[PGO] Training complete. Profile data captured." << std::endl;
}

// ==============================================================================
// Main Entrypoint & Failsafe Signal Handling
// ==============================================================================

static PowerTrayApp *g_app_instance = nullptr;

static void clean_exit_handler(int sig) {
    (void)sig;
    if (g_app_instance) {
        if (g_app_instance->get_current_mode() == "ultra") {
            std::string cmd = g_app_instance->get_manager_bin() + " restore --internal >/dev/null 2>&1 &";
            system(cmd.c_str());
        }
    }
    gtk_main_quit();
}

static int g_single_instance_lock_fd = -1;

static bool acquire_single_instance_lock() {
    std::string lock_path;
    const char *xdg_runtime = getenv("XDG_RUNTIME_DIR");
    if (xdg_runtime && xdg_runtime[0] != '\0') {
        lock_path = std::string(xdg_runtime) + "/thinkpower-tray.lock";
    } else {
        const char *home = getenv("HOME");
        if (home && home[0] != '\0') {
            lock_path = std::string(home) + "/.cache/thinkpower-tray.lock";
        } else {
            lock_path = "/tmp/thinkpower-tray-" + std::to_string(getuid()) + ".lock";
        }
    }

    g_single_instance_lock_fd = open(lock_path.c_str(), O_CREAT | O_RDWR | O_CLOEXEC, 0600);
    if (g_single_instance_lock_fd < 0) {
        return true;
    }

    if (flock(g_single_instance_lock_fd, LOCK_EX | LOCK_NB) != 0) {
        close(g_single_instance_lock_fd);
        g_single_instance_lock_fd = -1;
        return false;
    }

    if (ftruncate(g_single_instance_lock_fd, 0) == 0) {
        std::string pid_str = std::to_string(getpid()) + "\n";
        ssize_t w = write(g_single_instance_lock_fd, pid_str.c_str(), pid_str.size());
        (void)w;
    }
    return true;
}

int main(int argc, char **argv) {
    setlocale(LC_ALL, "");

    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--train" || std::string(argv[i]) == "--benchmark") {
            run_pgo_training(500000);
            return 0;
        }
    }

    // Prevent duplicate execution (Single-instance enforcement)
    if (!acquire_single_instance_lock()) {
        std::cout << "[ThinkPower] Another instance of power-tray is already running. Exiting cleanly." << std::endl;
        return 0;
    }

    gtk_init(&argc, &argv);
    PowerTrayApp app;
    g_app_instance = &app;
    signal(SIGINT, clean_exit_handler);
    signal(SIGTERM, clean_exit_handler);

    app.run();
    g_app_instance = nullptr;
    return 0;
}
