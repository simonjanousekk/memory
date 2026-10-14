#ifndef SIM_HTTPCLIENT_H
#define SIM_HTTPCLIENT_H

// HTTPClient shim. Requests are answered from fixtures, sent through libcurl,
// or refused, depending on the sim's network mode (sim/src/sim_net.cpp).

#include <WiFiClientSecure.h>

#include <string>
#include <utility>
#include <vector>

#define HTTPC_ERROR_CONNECTION_REFUSED (-1)
#define HTTPC_ERROR_READ_TIMEOUT (-11)

class HTTPClient {
 public:
  bool begin(WiFiClientSecure& /*client*/, const String& url) { return begin(url); }
  bool begin(const String& url) {
    _url = url.c_str();
    _headers.clear();
    _response.clear();
    return true;
  }
  void addHeader(const String& name, const String& value) { _headers.emplace_back(name.c_str(), value.c_str()); }
  void setConnectTimeout(int32_t ms) { _connect_timeout_ms = ms; }
  void setTimeout(uint32_t ms) { _timeout_ms = ms; }

  int GET() { return _request("GET", ""); }
  int POST(const String& body) { return _request("POST", body.c_str()); }
  String getString() { return String(_response); }
  void end() {}

 private:
  int _request(const char* method, const std::string& body);

  std::string _url;
  std::vector<std::pair<std::string, std::string>> _headers;
  std::string _response;
  int32_t _connect_timeout_ms = 5000;
  uint32_t _timeout_ms = 5000;
};

#endif
