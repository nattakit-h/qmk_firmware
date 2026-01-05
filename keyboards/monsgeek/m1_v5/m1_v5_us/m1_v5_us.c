// Copyright 2024 yangzheng20003 (@yangzheng20003)
// SPDX-License-Identifier: GPL-2.0-or-later

#include QMK_KEYBOARD_H
#include "data.h"
#include "indicators.h"
#include "wls/wls.h"

#include "wireless.h"
#include "usb_main.h"
#include "lowpower.h"

/*****************************************************************************/
/*                                  Data                                     */
/*****************************************************************************/

/* linker/wireless.c */
bool im_test_rate_flag = false;
/* linker/wireless/lpwr_wb32.c */
bool lower_sleep       = false;

mg_config_t mg_config = {0};
mg_data_t mg_data = {0};

/*****************************************************************************/
/*                                 Config                                    */
/*****************************************************************************/

void mg_config_write(void) {
    eeconfig_update_kb(mg_config.raw);
}

void mg_config_default(void) {
    mg_config = (mg_config_t){0};
    mg_config.initialized = true;
    mg_config.devs = DEVS_USB;
    mg_config_write();
}

void mg_config_init(void) {
    mg_config.raw = eeconfig_read_kb();
    if (!mg_config.raw) {
        mg_config_default();
    }
}

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

    hs_rgb_blink_set_timer(timer_read32());
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

uint32_t wls_process_long_press(uint32_t trigger_time, void *cb_arg) {
    uint16_t keycode = *((uint16_t *)cb_arg);

    switch (keycode) {
        case KC_BT1: {
            uint8_t mode = mg_config.devs;
            hs_modeio_detection(true, &mode);
            if ((mode == hs_bt) || (mode == hs_wireless) || (mode == hs_none)) {
                wireless_devs_change(wireless_get_current_devs(), DEVS_BT1, true);
            }

        } break;
        case KC_BT2: {
            uint8_t mode = mg_config.devs;
            hs_modeio_detection(true, &mode);
            if ((mode == hs_bt) || (mode == hs_wireless) || (mode == hs_none)) {
                wireless_devs_change(wireless_get_current_devs(), DEVS_BT2, true);
            }
        } break;
        case KC_BT3: {
            uint8_t mode = mg_config.devs;
            hs_modeio_detection(true, &mode);
            if ((mode == hs_bt) || (mode == hs_wireless) || (mode == hs_none)) {
                wireless_devs_change(wireless_get_current_devs(), DEVS_BT3, true);
            }
        } break;
        case KC_2G4: {
            uint8_t mode = mg_config.devs;
            hs_modeio_detection(true, &mode);
            if ((mode == hs_2g4) || (mode == hs_wireless) || (mode == hs_none)) {
                wireless_devs_change(wireless_get_current_devs(), DEVS_2G4, true);
            }
        } break;
        case EE_CLR: {

        } break;
        default:
            break;
    }

    return 0;
}

bool process_record_wls(uint16_t keycode, keyrecord_t *record) {
    // static uint16_t keycode_shadow                     = 0x00;
    static deferred_token wls_process_long_press_token = INVALID_DEFERRED_TOKEN;

    // keycode_shadow = keycode;

#    ifndef WLS_KEYCODE_PAIR_TIME
#        define WLS_KEYCODE_PAIR_TIME 3000
#    endif

#    define WLS_KEYCODE_EXEC(wls_dev)                                                                                          \
        do {                                                                                                                   \
            if (record->event.pressed) {                                                                                       \
                if (wireless_get_current_devs() != wls_dev)                                                                    \
                    wireless_devs_change(wireless_get_current_devs(), wls_dev, false);                                         \
                if (wls_process_long_press_token == INVALID_DEFERRED_TOKEN) {                                                  \
                    wls_process_long_press_token = defer_exec(WLS_KEYCODE_PAIR_TIME, wls_process_long_press, &keycode); \
                }                                                                                                              \
            } else {                                                                                                           \
                cancel_deferred_exec(wls_process_long_press_token);                                                            \
                wls_process_long_press_token = INVALID_DEFERRED_TOKEN;                                                         \
            }                                                                                                                  \
        } while (false)

    switch (keycode) {
        case KC_BT1: {
            uint8_t mode = mg_config.devs;
            hs_modeio_detection(true, &mode);
            if ((mode == hs_bt) || (mode == hs_wireless) || (mode == hs_none)) {
                WLS_KEYCODE_EXEC(DEVS_BT1);
                hs_rgb_blink_set_timer(timer_read32());
            }

        } break;
        case KC_BT2: {
            uint8_t mode = mg_config.devs;
            hs_modeio_detection(true, &mode);
            if ((mode == hs_bt) || (mode == hs_wireless) || (mode == hs_none)) {
                WLS_KEYCODE_EXEC(DEVS_BT2);
                hs_rgb_blink_set_timer(timer_read32());
            }
        } break;
        case KC_BT3: {
            uint8_t mode = mg_config.devs;
            hs_modeio_detection(true, &mode);
            if ((mode == hs_bt) || (mode == hs_wireless) || (mode == hs_none)) {
                WLS_KEYCODE_EXEC(DEVS_BT3);
                hs_rgb_blink_set_timer(timer_read32());
            }
        } break;
        case KC_2G4: {
            uint8_t mode = mg_config.devs;
            hs_modeio_detection(true, &mode);
            if ((mode == hs_2g4) || (mode == hs_wireless) || (mode == hs_none)) {
                WLS_KEYCODE_EXEC(DEVS_2G4);
                hs_rgb_blink_set_timer(timer_read32());
            }
        } break;

        case KC_USB: {
            uint8_t mode = mg_config.devs;
            hs_modeio_detection(true, &mode);
            if ((mode == hs_2g4) || (mode == hs_wireless) || (mode == hs_none)) {
                WLS_KEYCODE_EXEC(DEVS_USB);
                hs_rgb_blink_set_timer(timer_read32());
            }
        } break;
        default:
            return true;
    }

    return false;
}

bool process_record_kb(uint16_t keycode, keyrecord_t *record) {

    if (process_record_user(keycode, record) != true) {
        return false;
    }

    if (process_record_wls(keycode, record) != true) {
        return false;
    }

    switch (keycode) {
        case QK_BOOT: {
            if (record->event.pressed) {
                eeconfig_disable();
                bootloader_jump();
            }
        } break;
        case MG_TEST: {
            if (record->event.pressed) {
                // do noting
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

void housekeeping_task_user(void) { // loop
    if (mg_data.timestamp_reset && timer_elapsed32(mg_data.timestamp_reset) > 3000) {
        mg_state_reset();
        mg_data.timestamp_reset = 0;
    }

    static uint32_t hs_current_time;

    mg_data.usb_inserted = gpio_read_pin(MG_USB_INSERT_PIN);
    bool bat_fulled = gpio_read_pin(MG_BAT_FULL_PIN);

    if (mg_data.usb_inserted && bat_fulled) {
        mg_data.charge_state = MD_SND_CMD_DEVCTRL_CHARGING_DONE;
    } else if (mg_data.usb_inserted) {
        mg_data.charge_state = MD_SND_CMD_DEVCTRL_CHARGING;
    } else {
        mg_data.charge_state = MD_SND_CMD_DEVCTRL_CHARGING_STOP;
    }

    if (!hs_current_time || timer_elapsed32(hs_current_time) > 1000) {

        hs_current_time = timer_read32();
        md_send_devctrl(mg_data.charge_state);
        md_send_devctrl(MD_SND_CMD_DEVCTRL_INQVOL);
    }

    if (mg_data.usb_inserted) {
        gpio_write_pin_low(MG_LED_BOOST_PIN);

    } else {
        gpio_write_pin_high(MG_LED_BOOST_PIN);
    }
}

/*****************************************************************************/
/*                          RGB Matrix Overrides                             */
/*****************************************************************************/

bool rgb_matrix_indicators_kb(void) {
    mg_indicators_caplock();
    mg_indicators_guilock();
    mg_indicators_conn();
    mg_indicators_state();

    return true;
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
    hs_rgb_blink_set_timer(timer_read32());
}

/*****************************************************************************/
/*                          WestBerry Overrides                              */
/*****************************************************************************/

void usb_power_connect(void) {
    gpio_write_pin_low(MG_USB_POWER_PIN);
}

void usb_power_disconnect(void) {
    gpio_write_pin_high(MG_USB_POWER_PIN);
}

void wireless_devs_change_kb(uint8_t old_devs, uint8_t new_devs, bool reset) {
    if (mg_config.devs != wireless_get_current_devs()) {
        mg_config.devs = wireless_get_current_devs();
        mg_config_write();
    }

    mg_indicators_conn_start(new_devs, false);
}

void wireless_post_task(void) {
    // auto switching devs
    if (mg_data.timestamp_init && timer_elapsed32(mg_data.timestamp_init) >= 100) {

        md_send_devctrl(MD_SND_CMD_DEVCTRL_FW_VERSION);   // get the module fw version.
        md_send_devctrl(MD_SND_CMD_DEVCTRL_SLEEP_BT_EN);  // timeout 30min to sleep in bt mode, enable
        md_send_devctrl(MD_SND_CMD_DEVCTRL_SLEEP_2G4_EN); // timeout 30min to sleep in 2.4g mode, enable
        wireless_devs_change(!mg_config.devs, mg_config.devs, false);
        mg_data.timestamp_init = 0;
    }

    hs_mode_scan(false, mg_config.devs);

}

bool lpwr_is_allow_timeout_hook(void) {

    if (wireless_get_current_devs() == DEVS_USB) {
        return false;
    }

    return true;
}

bool lpwr_is_allow_presleep_hook(void) {
    if ((wireless_get_current_devs() == DEVS_USB) && !mg_data.usb_inserted) {

        if (USB_DRIVER.state != USB_STOP) {
            usb_power_disconnect();
            usbDisconnectBus(&USBD1);
            usbStop(&USBD1);
        }
    }
    return true;
}

void lpwr_exti_init_hook(void) {
    gpio_set_pin_input(MG_USB_INSERT_PIN);
    waitInputPinDelay();
    palEnableLineEvent(MG_USB_INSERT_PIN, PAL_EVENT_MODE_RISING_EDGE);

    setPinInput(ENCODER_B_PIN);
    waitInputPinDelay();
    palEnableLineEvent(ENCODER_B_PIN, PAL_EVENT_MODE_RISING_EDGE);
}


void lpwr_stop_hook_pre(void) {
    gpio_write_pin_low(MG_LED_POWER_PIN);
    gpio_write_pin_low(MG_LED_BOOST_PIN);
    gpio_write_pin_low(A9); // HACK: unknown pin
}

void lpwr_wakeup_hook(void) {
    hs_mode_scan(false, mg_config.devs);

    gpio_write_pin_high(MG_LED_POWER_PIN);
    gpio_write_pin_low(MG_LED_BOOST_PIN);
}

void palcallback_cb(uint8_t line) {
    switch (line) {
        case PAL_PAD(MG_USB_INSERT_PIN): {
            lpwr_set_sleep_wakeupcd(LPWR_WAKEUP_CABLE);
        } break;
        case PAL_PAD(ENCODER_B_PIN): {
            lpwr_set_sleep_wakeupcd(LPWR_WAKEUP_ENCODER);
        } break;
        default: {
        } break;
    }
}
