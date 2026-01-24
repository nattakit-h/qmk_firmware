#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "quantum.h"

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
    uint32_t timestamp_charge;
    uint32_t timestamp_rgb_timeout;
    uint32_t timestamp_connect_timeout;
    uint8_t  charge_state;
    bool     usb_inserted;
    bool     show_info;
    deferred_token sleep_exec;
} mg_data_t;

void mg_config_init(void);
void mg_config_default(void);
void mg_config_write(void);
