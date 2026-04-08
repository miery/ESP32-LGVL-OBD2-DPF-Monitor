#pragma once

#include <stdint.h>
#include <stdbool.h>

// Application state enum
typedef enum {
    STATE_DISCONNECTED,
    STATE_CONNECTED,
    STATE_INIT_OBD,
    STATE_READY
} app_state_t;

// Global state variables
extern app_state_t state;

extern uint16_t conn_handle;
extern uint16_t obd_char_handle;

// Parsed OBD values
extern int val_diff, val_temp, val_dist, val_soot, val_regen;
extern bool data_ready;