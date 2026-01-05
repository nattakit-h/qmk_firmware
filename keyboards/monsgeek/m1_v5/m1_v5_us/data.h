#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef union {
    uint32_t raw;
    struct {
        bool initialized : 1;
        uint8_t devs     : 3;
    };
} mg_config_t;

typedef struct {
    uint32_t timestamp_init;
    uint32_t timestamp_reset;
    uint8_t  charge_state;
    bool     usb_inserted;
    bool     show_info;
} mg_data_t;

enum MG_RGB_MATRIX_INDICES {
    KX_RALT,
    KX_FUNC,
    KX_RCTL,
    KX_LEFT,
    KX_DOWN,
    KX_RGHT,
    KX_END,
    KX_PGDN,
    KX_PGUP,
    KX_HOME,

    KX_DEL,
    KX_F12,
    KX_F11,
    KX_F10,
    KX_F9,
    KX_F8,
    KX_F7,
    KX_F6,
    KX_F5,
    KX_F4,
    KX_F3,
    KX_F2,
    KX_F1,
    KX_ESC,

    KX_GRV,
    KX_1,
    KX_2,
    KX_3,
    KX_4,
    KX_5,
    KX_6,
    KX_7,
    KX_8,
    KX_9,
    KX_0,
    KX_MINS,
    KX_EQL,
    KX_BSPC,

    KX_BSLS,
    KX_RBRC,
    KX_LBRC,
    KX_P,
    KX_O,
    KX_I,
    KX_U,
    KX_Y,
    KX_T,
    KX_R,
    KX_E,
    KX_W,
    KX_Q,
    KX_TAB,

    KX_CAPS,
    KX_A,
    KX_S,
    KX_D,
    KX_F,
    KX_G,
    KX_H,
    KX_J,
    KX_K,
    KX_L,
    KX_SCLN,
    KX_QUOT,
    KX_ENT,

    KX_UP,
    KX_RSFT,
    KX_SLSH,
    KX_DOT,
    KX_COMM,
    KX_M,
    KX_N,
    KX_B,
    KX_V,
    KX_C,
    KX_X,
    KX_Z,
    KX_LSFT,

    KX_LCTL,
    KX_LGUI,
    KX_LALT,
    KX_SPC
};
