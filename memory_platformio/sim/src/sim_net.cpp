// WiFi + HTTP emulation for the native simulator.
//
//   offline  WiFi never connects (tests the offline / failed path)
//   fixture  WiFi connects; GETs are answered from sim/fixtures/, POSTs are
//            printed and acknowledged without leaving the machine
//   live     WiFi connects; GETs go to Supabase through libcurl. POSTs are
//            still only printed unless --allow-upload is passed, so test runs
//            can't put fake players on the exhibition leaderboard.

#include <HTTPClient.h>
#include <WiFi.h>
#include <curl/curl.h>
#include <sim_config.h>

#include <atomic>
#include <chrono>
#include <fstream>
#include <sstream>
#include <thread>

#include "sim.h"

#ifndef SIM_DIR
#error "SIM_DIR must point at memory_platformio/sim (set by sim/sim_build.py)"
#endif

WiFiClass WiFi;

static std::atomic<bool> _wifi_available{false};
static std::atomic<bool> _wifi_begun{false};
static std::atomic<unsigned long> _wifi_begun_at{0};

// ---------------------------------------------------------------------------
// Sim controls
// ---------------------------------------------------------------------------
void sim_net_init() { _wifi_available = sim_opts.net != SIM_NET_OFFLINE; }

bool sim_wifi_available() { return _wifi_available; }

void sim_wifi_toggle() {
  _wifi_available = !_wifi_available;
  if (_wifi_available) _wifi_begun_at = millis();  // reconnecting takes time again
  printf("[sim] wifi %s\n", _wifi_available ? "available" : "unavailable");
}

const char* sim_net_mode_name() {
  switch (sim_opts.net) {
    case SIM_NET_OFFLINE:
      return "offline";
    case SIM_NET_FIXTURE:
      return "fixture";
    case SIM_NET_LIVE:
      return sim_opts.allow_upload ? "live+upload" : "live";
  }
  return "?";
}

// ---------------------------------------------------------------------------
// WiFiClass
// ---------------------------------------------------------------------------
wl_status_t WiFiClass::begin(const char* /*ssid*/, const char* /*passphrase*/) {
  _wifi_begun_at = millis();
  _wifi_begun = true;
  return WL_DISCONNECTED;
}

bool WiFiClass::disconnect(bool /*wifioff*/, bool /*eraseap*/) {
  _wifi_begun = false;
  return true;
}

bool WiFiClass::mode(wifi_mode_t mode) {
  if (mode == WIFI_OFF) _wifi_begun = false;
  return true;
}

wl_status_t WiFiClass::status() {
  if (!_wifi_begun) return WL_DISCONNECTED;
  if (!_wifi_available) return WL_NO_SSID_AVAIL;
  return millis() - _wifi_begun_at >= SIM_WIFI_CONNECT_MS ? WL_CONNECTED : WL_DISCONNECTED;
}

int8_t WiFiClass::RSSI() { return status() == WL_CONNECTED ? SIM_WIFI_RSSI : 0; }

IPAddress WiFiClass::localIP() {
  return status() == WL_CONNECTED ? IPAddress(192, 168, 0, 42) : IPAddress();
}

// ---------------------------------------------------------------------------
// HTTPClient
// ---------------------------------------------------------------------------
static size_t _curl_write(char* data, size_t size, size_t n, void* out) {
  static_cast<std::string*>(out)->append(data, size * n);
  return size * n;
}

static int _fixture_get(const std::string& url, std::string& response) {
  const char* file = nullptr;
  if (url.find("/rest/v1/players") != std::string::npos) file = SIM_DIR "/fixtures/leaderboard.json";
  if (!file) {
    printf("[sim] GET %s → 404 (no fixture)\n", url.c_str());
    return 404;
  }
  std::ifstream in(file);
  if (!in) {
    printf("[sim] fixture missing: %s\n", file);
    return 404;
  }
  std::stringstream ss;
  ss << in.rdbuf();
  response = ss.str();
  return 200;
}

int HTTPClient::_request(const char* method, const std::string& body) {
  _response.clear();
  if (WiFi.status() != WL_CONNECTED) return HTTPC_ERROR_CONNECTION_REFUSED;

  const bool is_post = strcmp(method, "POST") == 0;

  if (sim_opts.net != SIM_NET_LIVE || (is_post && !sim_opts.allow_upload)) {
    std::this_thread::sleep_for(std::chrono::milliseconds(SIM_FIXTURE_LATENCY_MS));
    if (is_post) {
      printf("[sim] POST %s not sent%s\n[sim] body: %s\n", _url.c_str(),
          sim_opts.net == SIM_NET_LIVE ? " (pass --allow-upload to send)" : "", body.c_str());
      return 200;
    }
    return _fixture_get(_url, _response);
  }

  CURL* curl = curl_easy_init();
  if (!curl) return HTTPC_ERROR_CONNECTION_REFUSED;

  struct curl_slist* headers = nullptr;
  for (const auto& h : _headers) headers = curl_slist_append(headers, (h.first + ": " + h.second).c_str());

  curl_easy_setopt(curl, CURLOPT_URL, _url.c_str());
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, _curl_write);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &_response);
  curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, (long)_connect_timeout_ms);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, (long)_timeout_ms);
  curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
  if (is_post) {
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)body.size());
  }

  int code;
  CURLcode rc = curl_easy_perform(curl);
  if (rc == CURLE_OK) {
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    code = (int)http_code;
  } else {
    printf("[sim] %s %s failed: %s\n", method, _url.c_str(), curl_easy_strerror(rc));
    code = rc == CURLE_OPERATION_TIMEDOUT ? HTTPC_ERROR_READ_TIMEOUT : HTTPC_ERROR_CONNECTION_REFUSED;
  }

  curl_slist_free_all(headers);
  curl_easy_cleanup(curl);
  return code;
}
