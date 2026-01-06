// Copyright 2024 yangzheng20003 (@yangzheng20003)
// SPDX-License-Identifier: GPL-2.0-or-later

#include QMK_KEYBOARD_H
#include "data.h"
#include "indicators.h"

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

mg_config_t mg_config  = {0};
mg_data_t   mg_data    = {0};

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
    lpwr_set_state(LPWR_STOP);
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
        case MG_TEST: {
            if (record->event.pressed && mg_data.sleep_exec == INVALID_DEFERRED_TOKEN) {
                mg_data.sleep_exec = defer_exec(500, mg_process_record_sleep_callback, NULL);
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
    if (mg_data.timestamp_init && timer_elapsed32(mg_data.timestamp_init) >= 100) {
        md_send_devctrl(MD_SND_CMD_DEVCTRL_FW_VERSION);
        md_send_devctrl(MD_SND_CMD_DEVCTRL_SLEEP_BT_EN);
        md_send_devctrl(MD_SND_CMD_DEVCTRL_SLEEP_2G4_EN);
        wireless_devs_change(!mg_config.devs, mg_config.devs, false);
        mg_data.timestamp_init = 0;
    }
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
    gpio_write_pin_high(MG_LED_POWER_PIN);
    gpio_write_pin_low(MG_LED_BOOST_PIN);
    mg_data.sleep_exec = INVALID_DEFERRED_TOKEN;
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
