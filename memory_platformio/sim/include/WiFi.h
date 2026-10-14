#ifndef SIM_WIFI_H
#define SIM_WIFI_H

// WiFi shim. Credentials are ignored; whether a connection succeeds depends
// on the sim's network mode and the W key (sim/src/sim_net.cpp).

#include <Arduino.h>

typedef enum {
  WL_NO_SHIELD = 255,
  WL_IDLE_STATUS = 0,
  WL_NO_SSID_AVAIL = 1,
  WL_SCAN_COMPLETED = 2,
  WL_CONNECTED = 3,
  WL_CONNECT_FAILED = 4,
  WL_CONNECTION_LOST = 5,
  WL_DISCONNECTED = 6,
} wl_status_t;

typedef enum {
  WIFI_OFF = 0,
  WIFI_STA = 1,
  WIFI_AP = 2,
  WIFI_AP_STA = 3,
} wifi_mode_t;

class IPAddress {
 public:
  IPAddress(uint8_t a = 0, uint8_t b = 0, uint8_t c = 0, uint8_t d = 0) : _b{a, b, c, d} {}
  uint8_t operator[](int i) const { return _b[i]; }
  String toString() const {
    char buf[16];
    snprintf(buf, sizeof(buf), "%u.%u.%u.%u", _b[0], _b[1], _b[2], _b[3]);
    return String(buf);
  }

 private:
  uint8_t _b[4];
};

class WiFiClass {
 public:
  wl_status_t begin(const char* ssid, const char* passphrase = nullptr);
  bool disconnect(bool wifioff = false, bool eraseap = false);
  bool mode(wifi_mode_t mode);
  wl_status_t status();
  int8_t RSSI();
  IPAddress localIP();
};

extern WiFiClass WiFi;

#endif
