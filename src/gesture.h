#pragma once
#include <stdbool.h>

// Initialize I2C and the APDS-9960 sensor.
// Returns true on success, false if the sensor is not found or wiring is wrong.
bool apds_init(void);

// Call repeatedly from a loop. Returns a direction string ("UP", "DOWN",
// "LEFT", "RIGHT") once per completed gesture, or NULL if no gesture is ready.
const char *read_gesture(void);
