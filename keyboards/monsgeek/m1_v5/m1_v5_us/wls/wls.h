#include QMK_KEYBOARD_H

#include "quantum.h"
#include <stdbool.h>
#include "wireless.h"

#define HS_MODEIO_DETECTION_TIME          50
#define HS_LBACK_TIMEOUT                  (30 * 1000)
#define HS_SLEEP_TIMEOUT                  (1 * 60000)

enum modeio_mode {
    hs_none = 0,
    hs_usb,
    hs_bt,
    hs_2g4,
    hs_wireless
};

bool hs_rgb_blink_hook(void);
bool hs_mode_scan(bool update, uint8_t mode);
bool hs_modeio_detection(bool update, uint8_t *mode);
void hs_rgb_blink_set_timer(uint32_t time);
