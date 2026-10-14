#ifndef SIM_ESP32ENCODER_H
#define SIM_ESP32ENCODER_H

// Encoder shim: the count is driven by the arrow keys in sim/src/sim_main.cpp.

#include <Arduino.h>

int64_t sim_encoder_count();

class ESP32Encoder {
 public:
  void attachHalfQuad(int /*a*/, int /*b*/) { _offset = sim_encoder_count(); }
  void attachFullQuad(int a, int b) { attachHalfQuad(a, b); }
  void attachSingleEdge(int a, int b) { attachHalfQuad(a, b); }
  int64_t getCount() { return sim_encoder_count() - _offset; }
  void clearCount() { _offset = sim_encoder_count(); }
  void setCount(int64_t value) { _offset = sim_encoder_count() - value; }

 private:
  int64_t _offset = 0;
};

#endif
