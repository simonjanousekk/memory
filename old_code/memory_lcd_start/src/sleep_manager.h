#ifndef SLEEP_MANAGER_H
#define SLEEP_MANAGER_H

#include <Adafruit_SharpMem.h>

// Initialize sleep manager.
//  - displayPowerPin: GPIO controlling power to the Sharp display
//  - switchPin: GPIO for the two-position sleep/wake switch (e.g. GPIO32)
void initSleep(int displayPowerPin, int switchPin, Adafruit_SharpMem &display);

// Call regularly from loop() so the switch can trigger sleep.
void checkButtonForSleep(Adafruit_SharpMem &display);

#endif
