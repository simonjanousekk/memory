#ifndef SIM_WIFICLIENTSECURE_H
#define SIM_WIFICLIENTSECURE_H

// TLS client stub. HTTPClient does the real work (sim/src/sim_net.cpp).

#include <WiFi.h>

class WiFiClientSecure {
 public:
  void setInsecure() {}
};

#endif
