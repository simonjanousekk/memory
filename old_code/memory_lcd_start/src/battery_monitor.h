#ifndef BATTERY_MONITOR_H
#define BATTERY_MONITOR_H

#include <Adafruit_GFX.h>
#include <Adafruit_SharpMem.h>

#define LIPO_FULL_V 4.2f
#define LIPO_EMPTY_V 3.2f

struct BatteryStatus {
  float voltageV;
  int percent;
  bool charging; // true if usbSensePin configured and USB present
  bool hasChargingPin;
};

void initBatteryMonitor(int vbatPin);
void updateBatteryStatus();
BatteryStatus getBatteryStatus();

// Draw battery info to any Adafruit_GFX target (e.g. GFXcanvas1)
void drawBatteryStatus(Adafruit_SharpMem &display);
#endif
