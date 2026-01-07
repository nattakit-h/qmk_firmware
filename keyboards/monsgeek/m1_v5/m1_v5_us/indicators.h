#pragma once

#include <stdint.h>
#include <color.h>


#define MG_INDICATORS_CONN_INDEX          KX_EQL
#define MG_INDICATORS_CONN_INTERVAL       500
#define MG_INDICATORS_CONN_INTERVAL_RESET 200
#define MG_INDICATORS_CONN_BT             RGB_BLUE
#define MG_INDICATORS_CONN_2G4            RGB_RED
#define MG_INDICATORS_CONN_USB            RGB_WHITE

#define MG_INDICATORS_BAT_CRITICAL      30
#define MG_INDICATORS_BAT_NORMAL        RGB_GREEN
#define MG_INDICATORS_BAT_LOW           RGB_RED
#define MG_INDICATORS_BAT_CHARGING      RGB_BLUE
#define MG_INDICATORS_BAT_CHARGING_DONE RGB_WHITE
#define MG_INDICATORS_BAT_INDICES       {KX_1, KX_2, KX_3, KX_4, KX_5, KX_6, KX_7, KX_8, KX_9, KX_0}

void mg_indicators_caplock(void);
void mg_indicators_guilock(void);
void mg_indicators_conn_start(int32_t dev_type, bool reset);
void mg_indicators_conn(bool show_connected);
void mg_indicators_state(void);
