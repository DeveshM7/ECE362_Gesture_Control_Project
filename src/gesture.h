#pragma once
#include <stdbool.h>
#include <stdint.h>

// ── Direction codes ───────────────────────────────────────────
#define DIR_UP    1
#define DIR_DOWN  2
#define DIR_LEFT  3
#define DIR_RIGHT 4

// Initialize I2C and the APDS-9960 sensor.
// Returns true on success, false if the sensor is not found or wiring is wrong.
bool apds_init(void);

// Call repeatedly from a loop. Returns a direction string ("UP", "DOWN",
// "LEFT", "RIGHT") once per completed gesture, or NULL if no gesture is ready.
const char *read_gesture(void);

// Core 1 entry point — runs the gesture detection loop.
// Pass to multicore_launch_core1() from main.
void core1_entry(void);
