#include "indicators.h"

#include "data.h"
#include "connection.h"
#include "rgb_indices.h"

#include "quantum.h"
#include "wireless.h"

extern mg_data_t mg_data;

/*****************************************************************************/
/*                               Definitions                                 */
/*****************************************************************************/

#define set_rgb(INDEX, RGB)                                                                 \
    do {                                                                                    \
        rgb_matrix_set_color(INDEX, ((rgb_t){RGB}).r, ((rgb_t){RGB}).g, ((rgb_t){RGB}).b);  \
        mg_data.timestamp_rgb_timeout = timer_read32();                                     \
    } while (0)

#define set_rgb_s(INDEX, RGB)                                    \
    do {                                                         \
        rgb_matrix_set_color(INDEX, (RGB).r, (RGB).g, (RGB).b);  \
        mg_data.timestamp_rgb_timeout = timer_read32();          \
    } while (0)

#define set_rgb_off(INDEX)                    \
    do {                                      \
        rgb_matrix_set_color(INDEX, 0, 0, 0); \
    } while (0)

/*****************************************************************************/
/*                               Processing                                  */
/*****************************************************************************/


void mg_indicators_caplock(void);
void mg_indicators_guilock(void);
void mg_indicators_conn(bool show_connected);
void mg_indicators_state(void);

void mg_process_indicators(void) {
    mg_indicators_conn(false);
    mg_indicators_state();

    // draw the locks after the state overlay so its wipe does not erase them
    mg_indicators_caplock();
    mg_indicators_guilock();

    // handle rgb sleep state
    const int32_t timeout = 3000;
    if (rgb_matrix_get_suspend_state()) {
        if (timer_elapsed32(mg_data.timestamp_rgb_timeout) < timeout) {
            rgb_matrix_set_suspend_state(false);
        }
    } else if (timer_elapsed32(mg_data.timestamp_rgb_timeout) > timeout) {
        rgb_matrix_set_suspend_state(true);
    }
}

/*****************************************************************************/
/*                            Basic Indicators                               */
/*****************************************************************************/

void mg_indicators_caplock(void) {
    static bool prev_caps = false;
    bool        caps      = host_keyboard_led_state().caps_lock;

    if (caps) {
        set_rgb(KX_CAPS, RGB_WHITE);
    } else if (prev_caps) {
        set_rgb_off(KX_CAPS);
    }

    prev_caps = caps;
}

void mg_indicators_guilock(void) {
    static bool prev_gui = false;
    bool        gui      = keymap_config.no_gui;

    if (gui) {
        set_rgb(KX_LGUI, RGB_WHITE);
    } else if (prev_gui) {
        set_rgb_off(KX_LGUI);
    }

    prev_gui = gui;
}

/*****************************************************************************/
/*                         Connectivity Indicator                            */
/*****************************************************************************/

const uint8_t mg_indicators_conn_index = KX_EQL;

// TODO: migrate these to mg_data and unify blinking routines
static rgb_t    mg_indicators_conn_rgb      = {RGB_WHITE}; // TODO: remove?
static uint32_t mg_indicators_conn_interval = 0; // FIXME: not working?
static uint32_t mg_indicators_conn_times    = 0;
static bool     mg_indicators_conn_reset    = false;

void mg_indicators_conn_set(uint8_t index, rgb_t rgb, uint32_t interval) {
    mg_data.timestamp_connect_blink    = timer_read32();
    mg_indicators_conn_rgb      = rgb;
    mg_indicators_conn_interval = interval;
    mg_indicators_conn_times    = 2;
}

rgb_t* mg_indicators_conn_color(uint8_t dev_type) {
    static rgb_t colors[] = {
        (rgb_t){ RGB_WHITE },
        (rgb_t){ RGB_BLUE },
        (rgb_t){ RGB_RED },
        (rgb_t){ RGB_MAGENTA }
    };
    switch (dev_type) {
        case DEVS_USB: { return &colors[0]; } break;
        case DEVS_BT1:
        case DEVS_BT2:
        case DEVS_BT3: { return &colors[1]; } break;
        case DEVS_2G4: { return &colors[2]; } break;
        default: { return &colors[3]; } break;;
    }
}

bool mg_indicators_conn_repeat(void) {
    if (mg_connection_actived()) {
        return false;
    }

    // still not connected, restart indicator
    mg_indicators_conn_start(wireless_get_current_devs(), mg_indicators_conn_reset);
    return true;
}

void mg_indicators_conn_start(int32_t dev_type, bool reset) {
    mg_indicators_conn_reset = reset;
    rgb_t* color = mg_indicators_conn_color(dev_type);
    uint32_t interval = reset ? 100 : 500;
    mg_indicators_conn_set(mg_indicators_conn_index, *color, interval);
}

void mg_indicators_conn(bool show_connected) {
    if (mg_data.timestamp_connect_blink != 0) {
        // interval elapsed
        if (timer_elapsed32(mg_data.timestamp_connect_blink) >= mg_indicators_conn_interval) {
            mg_data.timestamp_connect_blink = timer_read32();

            if (mg_indicators_conn_times > 0) {
                mg_indicators_conn_times--;
            }

            // ends timer
            if (mg_indicators_conn_times == 0) {
                mg_data.timestamp_connect_blink = 0;
                // repeat if still not connected
                if (!mg_indicators_conn_repeat()) {
                    return;
                }
            }
        }

        // blinking indicator light
        if (mg_indicators_conn_times % 2) {
            set_rgb_s(mg_indicators_conn_index, mg_indicators_conn_rgb);
        } else {
            set_rgb_off(mg_indicators_conn_index);
        }
    } else if (show_connected) {
        rgb_t* color = mg_indicators_conn_color(wireless_get_current_devs());
        set_rgb_s(mg_indicators_conn_index, *color);
    }
}

/*****************************************************************************/
/*                             State Indicator                               */
/*****************************************************************************/

void mg_indicators_state(void) {
    extern mg_data_t mg_data;
    // show the overlay while the info key is held, or continuously in debug mode
    if (!mg_data.show_info && !mg_data.debug_mode) return;

    rgb_matrix_set_color_all(0, 0, 0);

    // battery bar only with a fresh reading; unknown/stale = all off
    const uint32_t battery_fresh_window = 10000;
    const bool     battery_fresh        = (mg_data.timestamp_battery != 0) && (timer_elapsed32(mg_data.timestamp_battery) <= battery_fresh_window);

    if (battery_fresh) {
        uint8_t battery = *md_getp_bat();
        const uint8_t mg_indicators_battery_indices[] = {KX_1, KX_2, KX_3, KX_4, KX_5, KX_6, KX_7, KX_8, KX_9, KX_0};
        for (uint8_t i = 0; i < ARRAY_SIZE(mg_indicators_battery_indices); i++) {
            if ((i < (battery / 10)) || (i == 0)) {
                if (mg_data.charge_state == MD_SND_CMD_DEVCTRL_CHARGING) {
                    set_rgb(mg_indicators_battery_indices[i], RGB_BLUE);
                } else if (mg_data.charge_state == MD_SND_CMD_DEVCTRL_CHARGING_DONE) {
                    set_rgb(mg_indicators_battery_indices[i], RGB_WHITE);
                } else if (battery >= 30) {
                    set_rgb(mg_indicators_battery_indices[i], RGB_GREEN);
                } else {
                    set_rgb(mg_indicators_battery_indices[i], RGB_RED);
                }
            } else {
                set_rgb_off(mg_indicators_battery_indices[i]);
            }
        }
    }

    mg_indicators_conn(true);

    const uint8_t conn_state_indicator = KX_MINS;
    if (!mg_connection_actived()) {
        set_rgb(conn_state_indicator, RGB_RED);
    } else if (!keymap_config.nkro) {
        set_rgb(conn_state_indicator, RGB_GOLD);
    } else {
        set_rgb(conn_state_indicator, RGB_GREEN);
    }
}
