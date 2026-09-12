#pragma once

#include <stdbool.h>

#define MG_CONNECTION_TIMEOUT (30 * 1000)
#define MG_SLEEP_TIMEOUT      (1 * 60 * 1000)

bool mg_connection_actived(void);
bool mg_connection_usb_actived(void);

// inject the detent that woke the board, once the link is live
void encoder_deliver_pending(void);
