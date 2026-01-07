#include "indicators.h"

#include "data.h"
#include "connection.h"
#include "rgb_indices.h"

#include "quantum.h"
#include "wireless.h"

#define set_rgb(INDEX, RGB)   \
    do {                      \
        rgb_matrix_set_color(INDEX, ((rgb_t){RGB}).r, ((rgb_t){RGB}).g, ((rgb_t){RGB}).b); \
    } while (0)               \

#define set_rgb_s(INDEX, RGB) \
    do {                      \
        rgb_matrix_set_color(INDEX, (RGB).r, (RGB).g, (RGB).b); \
    } while (0)

/*****************************************************************************/
/*                            Basic Indicators                               */
/*****************************************************************************/

void mg_indicators_caplock(void) {
    if (host_keyboard_led_state().caps_lock) {
        set_rgb(KX_CAPS, RGB_WHITE);
    }
}

void mg_indicators_guilock(void) {
    if (keymap_config.no_gui) {
        set_rgb(KX_LGUI, RGB_WHITE);
    }
}

/*****************************************************************************/
/*                         Connectivity Indicator                            */
/*****************************************************************************/
// TODO: migrate these to mg_data
static uint32_t mg_indicators_conn_timer    = 0;
static uint8_t  mg_indicators_conn_index    = 0;
static rgb_t    mg_indicators_conn_rgb      = {RGB_WHITE};
static uint32_t mg_indicators_conn_interval = 0;
static uint32_t mg_indicators_conn_times    = 0;
static bool     mg_indicators_conn_reset    = false;

static void mg_indicators_conn_set(uint8_t index, rgb_t rgb, uint32_t interval) {
    mg_indicators_conn_timer    = timer_read32();
    mg_indicators_conn_index    = index;
    mg_indicators_conn_rgb      = rgb;
    mg_indicators_conn_interval = interval;
    mg_indicators_conn_times    = 2;
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
    uint32_t interval = reset ? MG_INDICATORS_CONN_INTERVAL : MG_INDICATORS_CONN_INTERVAL_RESET;
    switch (dev_type) {
        case DEVS_USB: {
            mg_indicators_conn_set(MG_INDICATORS_CONN_INDEX, (rgb_t){MG_INDICATORS_CONN_USB}, interval);
        } break;
        case DEVS_BT1: {
            mg_indicators_conn_set(MG_INDICATORS_CONN_INDEX, (rgb_t){MG_INDICATORS_CONN_BT}, interval);
        } break;
        case DEVS_BT2: {
            mg_indicators_conn_set(MG_INDICATORS_CONN_INDEX, (rgb_t){MG_INDICATORS_CONN_BT}, interval);
        } break;
        case DEVS_BT3: {
            mg_indicators_conn_set(MG_INDICATORS_CONN_INDEX, (rgb_t){MG_INDICATORS_CONN_BT}, interval);
        } break;
        case DEVS_2G4: {
            mg_indicators_conn_set(MG_INDICATORS_CONN_INDEX, (rgb_t){MG_INDICATORS_CONN_2G4}, interval);
        } break;
        default:
            break;
    }
}

void mg_indicators_conn(bool show_connected) {
    if (mg_indicators_conn_timer != 0) {
        // interval elapsed
        if (timer_elapsed32(mg_indicators_conn_timer) >= mg_indicators_conn_interval) {
            mg_indicators_conn_timer = timer_read32();

            if (mg_indicators_conn_times > 0) {
                mg_indicators_conn_times--;
            }

            // ends timer
            if (mg_indicators_conn_times == 0) {
                mg_indicators_conn_timer = 0;
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
            set_rgb(mg_indicators_conn_index, RGB_BLACK);
        }
    } else if (show_connected) {
        switch (wireless_get_current_devs()) {
            case DEVS_2G4: {
                set_rgb(MG_INDICATORS_CONN_INDEX, MG_INDICATORS_CONN_2G4);
            } break;
            case DEVS_USB: {
                set_rgb(MG_INDICATORS_CONN_INDEX, MG_INDICATORS_CONN_USB);
            } break;
            case DEVS_BT1:
            case DEVS_BT2:
            case DEVS_BT3: {
                set_rgb(MG_INDICATORS_CONN_INDEX, MG_INDICATORS_CONN_BT);
            } break;
            default: {
            } break;
        }
    }
}

/*****************************************************************************/
/*                             State Indicator                               */
/*****************************************************************************/

void mg_indicators_state(void) {
    extern mg_data_t mg_data;
    if (!mg_data.show_info) return;

    rgb_matrix_set_color_all(0, 0, 0);

    const uint8_t mg_indicators_battery_indices[] = MG_INDICATORS_BAT_INDICES;
    for (uint8_t i = 0; i < ARRAY_SIZE(mg_indicators_battery_indices); i++) {
        if ((i < (*md_getp_bat() / 10)) || (i == 0)) {
                if (mg_data.charge_state == MD_SND_CMD_DEVCTRL_CHARGING) {
                    set_rgb(mg_indicators_battery_indices[i], MG_INDICATORS_BAT_CHARGING);
                } else if (mg_data.charge_state == MD_SND_CMD_DEVCTRL_CHARGING_DONE) {
                    set_rgb(mg_indicators_battery_indices[i], MG_INDICATORS_BAT_CHARGING_DONE);
                } else if (*md_getp_bat() >= (MG_INDICATORS_BAT_CRITICAL)) {
                    set_rgb(mg_indicators_battery_indices[i], MG_INDICATORS_BAT_NORMAL);
                } else {
                    set_rgb(mg_indicators_battery_indices[i], MG_INDICATORS_BAT_LOW);
                }
            } else {
                set_rgb(mg_indicators_battery_indices[i], RGB_OFF);
        }
    }

    mg_indicators_conn(true);

    const uint8_t conn_state_indicator = KX_MINS;
    if (mg_connection_actived()) {
        set_rgb(conn_state_indicator, RGB_GREEN);
    } else {
        set_rgb(conn_state_indicator, RGB_RED);
    }
}
