#include "data.h"

#include "wireless.h"

mg_config_t mg_config  = {0};
mg_data_t   mg_data    = {0};

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
