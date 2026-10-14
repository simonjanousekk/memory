#ifndef SIM_WIRE_H
#define SIM_WIRE_H

// I2C shim. The only I2C device (MAX17048) is stubbed at the library level.

#include <Arduino.h>

class TwoWire {
 public:
  bool begin(int /*sda*/ = -1, int /*scl*/ = -1, uint32_t /*freq*/ = 0) { return true; }
};

extern TwoWire Wire;

#endif
