#include <stdbool.h>
#include <stdint.h>

#include "connection.h"

#include "data.h"
#include "indicators.h"

#include "wireless.h"
#include "usb_main.h"
#include "quantum.h"

extern mg_data_t mg_data;
extern mg_config_t mg_config;

bool mg_connection_usb_actived(void) {
    return wireless_get_current_devs() == DEVS_USB && USB_DRIVER.state == USB_ACTIVE;
}

bool mg_connection_actived(void) {
    if (mg_connection_usb_actived()) {
        return true;
    }

    // wireless connected
    if (*md_getp_state() == MD_STATE_CONNECTED) {
        return true;
    }

    return false;
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

    mg_data.timestamp_connect_timeout = timer_read32();

    // battery reading from the previous mode is stale
    mg_data.timestamp_battery = 0;

    mg_indicators_conn_start(new_devs, reset);

}

bool md_receive_process_kb(uint8_t *pdata, uint8_t len) {
    // stamp freshness when a battery report (0x5C) passes by,
    // then let the vendor handler store the value
    if (pdata[0] == MD_REV_CMD_BATVOL) {
        mg_data.timestamp_battery = timer_read32();
    }
    return true;
}

void wireless_post_task(void) {
    if (mg_data.timestamp_init && timer_elapsed32(mg_data.timestamp_init) >= 100) {
        md_send_devctrl(MD_SND_CMD_DEVCTRL_FW_VERSION);
        md_send_devctrl(MD_SND_CMD_DEVCTRL_SLEEP_BT_EN);
        md_send_devctrl(MD_SND_CMD_DEVCTRL_SLEEP_2G4_EN);
        wireless_devs_change(!mg_config.devs, mg_config.devs, false);
        // protocol_post_init() re-installs chibios_driver after boot, and the
        // re-select above can't fix it (set_transport is a no-op here)
        if (get_transport() == TRANSPORT_WLS) {
            wls_transport_enable(true);
        }
        mg_data.timestamp_init = 0;
    }

    /* sleep timeout */
    if (mg_connection_actived()) {
        mg_data.timestamp_connect_timeout = 0;
        if (last_input_activity_elapsed() > MG_SLEEP_TIMEOUT) {
            lpwr_set_timeout_manual(true);
        }
    /* connection timeout */
    } else {
        if (mg_data.timestamp_connect_timeout == 0) mg_data.timestamp_connect_timeout = timer_read32();
        if (timer_elapsed32(mg_data.timestamp_connect_timeout) >= MG_CONNECTION_TIMEOUT) {
            lpwr_set_timeout_manual(true);
        }
    }
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

    gpio_set_pin_input_high(ENCODER_B_PIN);
    waitInputPinDelay();
    palEnableLineEvent(ENCODER_B_PIN, PAL_EVENT_MODE_RISING_EDGE);
}


void lpwr_stop_hook_pre(void) {
    gpio_write_pin_low(MG_LED_POWER_PIN);
    gpio_write_pin_low(MG_LED_BOOST_PIN);
    gpio_write_pin_low(A9); // HACK: unknown pin
}

// direction of the detent that woke the board, from the wake ISR
static bool encoder_wake_clockwise = false;

// set by the encoder wake ISR. Not keyed off lpwr_wakeupcd: a UART edge can
// overwrite that to LPWR_WAKEUP_UART and drop the board back to stop.
static volatile bool encoder_wake_seen = false;

// held until the link is live, then injected (else the report is dropped)
static volatile bool encoder_pending_dir = false;
static volatile bool encoder_pending     = false;

void lpwr_stop_hook_post(void) {
    if (encoder_wake_seen) {
        encoder_wake_seen = false;
        encoder_init(); // resync to present pins
        // inject only if already at rest (detent finished during sleep);
        // otherwise the driver emits it when the detent completes
        if (gpio_read_pin(ENCODER_A_PIN) && gpio_read_pin(ENCODER_B_PIN)) {
            encoder_pending_dir = encoder_wake_clockwise;
            encoder_pending     = true;
        }
    }
}

// inject the held detent once connected, so the report isn't dropped
void encoder_deliver_pending(void) {
    if (!encoder_pending || !mg_connection_actived()) {
        return;
    }
    encoder_pending = false;
    encoder_queue_event(0, encoder_pending_dir);
}

void lpwr_wakeup_hook(void) {
    gpio_write_pin_high(MG_LED_POWER_PIN);
    gpio_write_pin_high(MG_LED_BOOST_PIN);
    mg_data.sleep_exec = INVALID_DEFERRED_TOKEN;

    // module re-establishes its link across sleep; old reading is stale
    mg_data.timestamp_battery = 0;
}

void palcallback_cb(uint8_t line) {
    // if/else rather than switch: ENCODER_B_PIN is a compound literal, not an
    // integer constant expression, so it can't be a case label
    if (line == PAL_PAD(MG_USB_INSERT_PIN)) {
        lpwr_set_sleep_wakeupcd(LPWR_WAKEUP_CABLE);
    } else if (line == PAL_PAD(ENCODER_B_PIN)) {
        lpwr_set_sleep_wakeupcd(LPWR_WAKEUP_ENCODER);
        encoder_wake_seen = true;
        // at a B-rising edge A is stable: A=1 is clockwise (see encoder_LUT)
        encoder_wake_clockwise = (gpio_read_pin(ENCODER_A_PIN) != 0);
    }
}
