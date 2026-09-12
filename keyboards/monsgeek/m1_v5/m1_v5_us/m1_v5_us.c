// Copyright 2024 yangzheng20003 (@yangzheng20003)
// SPDX-License-Identifier: GPL-2.0-or-later

#include QMK_KEYBOARD_H
#include "connection.h"
#include "data.h"
#include "indicators.h"

#include "wireless.h"

/*****************************************************************************/
/*                                  Data                                     */
/*****************************************************************************/

/* linker/wireless.c */
bool im_test_rate_flag = false;
/* linker/wireless/lpwr_wb32.c */
bool lower_sleep       = false;

extern mg_data_t mg_data;
extern mg_config_t mg_config;

/*****************************************************************************/
/*                             Initialization                                */
/*****************************************************************************/

void mg_state_reset(void) {
    eeconfig_init();
    eeconfig_update_rgb_matrix_default();
    eeconfig_read_keymap(&keymap_config);

    mg_data = (mg_data_t){0};

#if defined(NKRO_ENABLE)
    keymap_config.nkro = 0;
    eeconfig_update_keymap(&keymap_config);
#endif

    keyboard_post_init_kb();
}

void keyboard_post_init_kb(void) {

#ifdef CONSOLE_ENABLE
    debug_enable = true;
#endif

    mg_config_init();

    gpio_set_pin_output(MG_LED_POWER_PIN);
    gpio_write_pin_high(MG_LED_POWER_PIN);
    gpio_set_pin_output(MG_LED_BOOST_PIN);
    gpio_write_pin_high(MG_LED_BOOST_PIN);

    gpio_write_pin_low(MG_USB_POWER_PIN);
    gpio_set_pin_output(MG_USB_POWER_PIN);

    gpio_set_pin_input(MG_USB_INSERT_PIN);
    gpio_set_pin_input_high(MG_BAT_FULL_PIN);

    gpio_set_pin_input_high(MG_USE_BAT_PIN);
    gpio_set_pin_input_high(MG_USE_MAC_PIN);

    wireless_init();
    wireless_devs_change(!mg_config.devs, mg_config.devs, false);
    mg_data.timestamp_init = timer_read32();

    keyboard_post_init_user();
}


/*****************************************************************************/
/*                             Keys Processing                               */
/*****************************************************************************/

uint32_t mg_process_record_connect_reset_callback(uint32_t trigger_time, void *cb_arg) {
    wireless_devs_change(wireless_get_current_devs(), (uint32_t)cb_arg, true);
    return 0;
}

bool mg_process_record_connect(uint16_t keycode, keyrecord_t *record) {
    const uint32_t hold_time = 3000;
    static deferred_token reset_exec = INVALID_DEFERRED_TOKEN;

    uint32_t dev = 0;
    switch (keycode) {
        case MG_USB: dev = DEVS_USB; break;
        case MG_2G4: dev = DEVS_2G4; break;
        case MG_BT1: dev = DEVS_BT1; break;
        case MG_BT2: dev = DEVS_BT2; break;
        case MG_BT3: dev = DEVS_BT3; break;
        default: return true;
    }

    if (record->event.pressed) {
        if (wireless_get_current_devs() != dev) {
            wireless_devs_change(wireless_get_current_devs(), dev, false);
        }
        if (reset_exec == INVALID_DEFERRED_TOKEN) {
            reset_exec = defer_exec(hold_time, mg_process_record_connect_reset_callback, (void*)dev);
        }
    } else {
        cancel_deferred_exec(reset_exec);
        reset_exec = INVALID_DEFERRED_TOKEN;
    }

    return false;
}

uint32_t mg_process_record_sleep_callback(uint32_t trigger_time, void *cb_arg) {
    lpwr_set_state(LPWR_PRESLEEP);
    return 0;
}

bool process_record_kb(uint16_t keycode, keyrecord_t *record) {

    if (process_record_user(keycode, record) != true) {
        return false;
    }

    if (mg_process_record_connect(keycode, record) != true) {
        return false;
    }

    switch (keycode) {
        case QK_BOOT: {
            if (record->event.pressed) {
                eeconfig_disable();
                bootloader_jump();
            }
        } break;
        case MG_SLP: {
            // refuse in USB-active: the host holds the board awake there
            if (record->event.pressed && !mg_connection_usb_actived() && mg_data.sleep_exec == INVALID_DEFERRED_TOKEN) {
                mg_data.sleep_exec = defer_exec(500, mg_process_record_sleep_callback, NULL);
            }
            return false;
        } break;
        case MG_DBG: {
            // toggle the always-on status overlay
            if (record->event.pressed) {
                mg_data.debug_mode = !mg_data.debug_mode;
            }
            return false;
        } break;
        case EE_CLR: {
            if (record->event.pressed) {
                mg_data.timestamp_reset = timer_read32();
            } else {
                mg_data.timestamp_reset = 0;
            }

            return false;
        } break;
        case MG_INFO: {
            mg_data.show_info = record->event.pressed;
            return false;
        } break;
        default:
            break;
    }

    return true;
}

void housekeeping_task_user(void) {
    if (mg_data.timestamp_reset && timer_elapsed32(mg_data.timestamp_reset) > 3000) {
        mg_state_reset();
        mg_data.timestamp_reset = 0;
    }

    mg_data.usb_inserted = gpio_read_pin(MG_USB_INSERT_PIN);
    bool bat_fulled = gpio_read_pin(MG_BAT_FULL_PIN);

    if (mg_data.usb_inserted && bat_fulled) {
        mg_data.charge_state = MD_SND_CMD_DEVCTRL_CHARGING_DONE;
    } else if (mg_data.usb_inserted) {
        mg_data.charge_state = MD_SND_CMD_DEVCTRL_CHARGING;
    } else {
        mg_data.charge_state = MD_SND_CMD_DEVCTRL_CHARGING_STOP;
    }

    if (mg_data.timestamp_charge == 0 || timer_elapsed32(mg_data.timestamp_charge) > 1000) {
        mg_data.timestamp_charge = timer_read32();
        md_send_devctrl(mg_data.charge_state);
    }

    // poll battery; md_inquire_bat() refuses while the smsg queue is busy,
    // so leave the timestamp untouched and retry on the next pass
    if (mg_data.timestamp_battery_query == 0 || timer_elapsed32(mg_data.timestamp_battery_query) > 1000) {
        if (md_inquire_bat()) {
            mg_data.timestamp_battery_query = timer_read32();
        }
    }

    if (mg_data.usb_inserted) {
        gpio_write_pin_low(MG_LED_BOOST_PIN);
    } else {
        gpio_write_pin_high(MG_LED_BOOST_PIN);
    }

    encoder_deliver_pending();

    // process indicators here instead of rgb_matrix_indicators_kb to handle
    // rgb sleeping state
    mg_process_indicators();
}

/*****************************************************************************/
/*                            Suspend Overrides                              */
/*****************************************************************************/

void suspend_power_down_kb(void) {
    gpio_write_pin_low(MG_LED_POWER_PIN);
    suspend_power_down_user();
}

void suspend_wakeup_init_kb(void) {
    gpio_write_pin_high(MG_LED_POWER_PIN);

    wireless_devs_change(wireless_get_current_devs(), wireless_get_current_devs(), false);
    suspend_wakeup_init_user();
}
