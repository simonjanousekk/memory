#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <display.h>
#include <wifi/wifi_credentials.h>

#define WIFI_PER_NETWORK_TIMEOUT_MS 4000

// ---------------------------------------------------------------------------
// State
// ---------------------------------------------------------------------------
enum WifiTestState {
  WIFI_TEST_IDLE,
  WIFI_TEST_CONNECTING,
  WIFI_TEST_CONNECTED,
  WIFI_TEST_FAILED,
};

static WifiTestState _wifi_state = WIFI_TEST_IDLE;
static unsigned long _wifi_start_ms = 0;
static int _wifi_net_idx = 0;
static int _wifi_rssi = 0;
static char _wifi_ip[16] = "";
static char _wifi_ssid[33] = "";

// ---------------------------------------------------------------------------
// Internal — begin connecting to network at _wifi_net_idx
// ---------------------------------------------------------------------------
static void _wifi_try_network(int idx) {
  WiFi.disconnect(true);
  delay(10);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_NETWORKS[idx].ssid, WIFI_NETWORKS[idx].password);
  strncpy(_wifi_ssid, WIFI_NETWORKS[idx].ssid, sizeof(_wifi_ssid) - 1);
  _wifi_ssid[sizeof(_wifi_ssid) - 1] = '\0';
  _wifi_start_ms = millis();
}

// ---------------------------------------------------------------------------
// Control — called from DebugMenuItem on_enable / on_disable
// ---------------------------------------------------------------------------
void wifi_manager_start() {
  if (_wifi_state == WIFI_TEST_CONNECTING)
    return;
  _wifi_net_idx = 0;
  _wifi_ip[0] = '\0';
  _wifi_rssi = 0;
  _wifi_state = WIFI_TEST_CONNECTING;
  _wifi_try_network(0);
}

void wifi_manager_reset() {
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  _wifi_state = WIFI_TEST_IDLE;
  _wifi_net_idx = 0;
  _wifi_ip[0] = '\0';
  _wifi_ssid[0] = '\0';
  _wifi_rssi = 0;
}

// ---------------------------------------------------------------------------
// Update — call every loop(); no-op unless actively connecting.
// WiFi.begin() is async — the stack runs on Core 0, this just polls status.
// ---------------------------------------------------------------------------
void wifi_manager_update() {
  if (_wifi_state != WIFI_TEST_CONNECTING)
    return;

  if (WiFi.status() == WL_CONNECTED) {
    _wifi_state = WIFI_TEST_CONNECTED;
    _wifi_rssi = WiFi.RSSI();
    IPAddress ip = WiFi.localIP();
    snprintf(_wifi_ip, sizeof(_wifi_ip), "%u.%u.%u.%u", ip[0], ip[1], ip[2],
             ip[3]);
    return;
  }

  if (millis() - _wifi_start_ms >= WIFI_PER_NETWORK_TIMEOUT_MS) {
    _wifi_net_idx++;
    if (_wifi_net_idx < WIFI_NETWORK_COUNT) {
      _wifi_try_network(_wifi_net_idx);
    } else {
      _wifi_state = WIFI_TEST_FAILED;
      WiFi.disconnect(true);
    }
  }
}

// ---------------------------------------------------------------------------
// Debug overlay — render callback for DebugMenuItem
// ---------------------------------------------------------------------------
// Returns true once all networks have been tried and none succeeded.
bool wifi_manager_failed() { return _wifi_state == WIFI_TEST_FAILED; }

void wifi_debug_display(char *buf, int len) {
  switch (_wifi_state) {
  case WIFI_TEST_IDLE:
    snprintf(buf, len, "WiFi: idle");
    break;
  case WIFI_TEST_CONNECTING: {
    int elapsed = (int)((millis() - _wifi_start_ms) / 1000);
    snprintf(buf, len, "WiFi %d/%d %.16s %ds", _wifi_net_idx + 1,
             WIFI_NETWORK_COUNT, _wifi_ssid, elapsed);
    break;
  }
  case WIFI_TEST_CONNECTED:
    snprintf(buf, len, "WiFi OK %.12s %ddBm %s", _wifi_ssid, _wifi_rssi,
             _wifi_ip);
    break;
  case WIFI_TEST_FAILED:
    snprintf(buf, len, "WiFi: all %d failed", WIFI_NETWORK_COUNT);
    break;
  }
}

#endif
