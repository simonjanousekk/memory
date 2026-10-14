#ifndef SIM_ADAFRUIT_MAX1704X_H
#define SIM_ADAFRUIT_MAX1704X_H

// MAX17048 fuel gauge stub with fixed readings (see sim/sim_config.h).

#include <Wire.h>
#include <sim_config.h>

class Adafruit_MAX17048 {
 public:
  bool begin(TwoWire* /*wire*/ = &Wire) { return true; }
  void quickStart() {}
  float cellVoltage() { return SIM_BATTERY_VOLTAGE; }
  float cellPercent() { return SIM_BATTERY_PERCENT; }
};

#endif
