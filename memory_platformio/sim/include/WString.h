#ifndef SIM_WSTRING_H
#define SIM_WSTRING_H

// Minimal Arduino String for the native simulator, backed by std::string.
// Only the API the firmware, Adafruit GFX and ArduinoJson actually use.

#include <stdio.h>

#include <string>

// Only declared: F() strings are plain const char* in the sim.
class __FlashStringHelper;

class String {
 public:
  String() {}
  String(const char* s) : _s(s ? s : "") {}
  String(const std::string& s) : _s(s) {}
  explicit String(char c) : _s(1, c) {}
  explicit String(int v, unsigned char base = 10) : _s(_fmt_int((long long)v, base)) {}
  explicit String(unsigned int v, unsigned char base = 10) : _s(_fmt_uint(v, base)) {}
  explicit String(long v, unsigned char base = 10) : _s(_fmt_int(v, base)) {}
  explicit String(unsigned long v, unsigned char base = 10) : _s(_fmt_uint(v, base)) {}
  explicit String(long long v, unsigned char base = 10) : _s(_fmt_int(v, base)) {}
  explicit String(unsigned long long v, unsigned char base = 10) : _s(_fmt_uint(v, base)) {}
  explicit String(float v, unsigned int decimals = 2) : _s(_fmt_double(v, decimals)) {}
  explicit String(double v, unsigned int decimals = 2) : _s(_fmt_double(v, decimals)) {}

  unsigned int length() const { return (unsigned int)_s.size(); }
  bool isEmpty() const { return _s.empty(); }
  const char* c_str() const { return _s.c_str(); }
  bool reserve(unsigned int size) {
    _s.reserve(size);
    return true;
  }

  bool concat(const String& s) {
    _s += s._s;
    return true;
  }
  bool concat(const char* s) {
    if (s) _s += s;
    return true;
  }
  bool concat(const char* s, unsigned int n) {
    if (s) _s.append(s, n);
    return true;
  }
  bool concat(char c) {
    _s += c;
    return true;
  }

  String& operator+=(const String& s) { return concat(s), *this; }
  String& operator+=(const char* s) { return concat(s), *this; }
  String& operator+=(char c) { return concat(c), *this; }

  friend String operator+(const String& a, const String& b) { return String(a._s + b._s); }
  friend String operator+(const String& a, const char* b) { return String(a._s + (b ? b : "")); }
  friend String operator+(const char* a, const String& b) { return String((a ? a : "") + b._s); }
  friend String operator+(const String& a, char c) { return String(a._s + c); }

  bool operator==(const String& o) const { return _s == o._s; }
  bool operator==(const char* o) const { return _s == (o ? o : ""); }
  bool operator!=(const String& o) const { return !(*this == o); }
  bool operator!=(const char* o) const { return !(*this == o); }
  bool operator<(const String& o) const { return _s < o._s; }

  char charAt(unsigned int i) const { return i < _s.size() ? _s[i] : 0; }
  char operator[](unsigned int i) const { return charAt(i); }
  char& operator[](unsigned int i) { return _s[i]; }

  void remove(unsigned int index) {
    if (index < _s.size()) _s.erase(index);
  }
  void remove(unsigned int index, unsigned int count) {
    if (index < _s.size()) _s.erase(index, count);
  }
  String substring(unsigned int from) const { return from < _s.size() ? String(_s.substr(from)) : String(); }
  String substring(unsigned int from, unsigned int to) const {
    if (from > to) std::swap(from, to);
    return from < _s.size() ? String(_s.substr(from, to - from)) : String();
  }
  int indexOf(char c, unsigned int from = 0) const {
    size_t i = _s.find(c, from);
    return i == std::string::npos ? -1 : (int)i;
  }
  int indexOf(const String& s, unsigned int from = 0) const {
    size_t i = _s.find(s._s, from);
    return i == std::string::npos ? -1 : (int)i;
  }
  bool startsWith(const String& s) const { return _s.rfind(s._s, 0) == 0; }
  void toUpperCase() {
    for (char& c : _s) c = (char)toupper((unsigned char)c);
  }
  void toLowerCase() {
    for (char& c : _s) c = (char)tolower((unsigned char)c);
  }
  long toInt() const { return strtol(_s.c_str(), nullptr, 10); }
  float toFloat() const { return strtof(_s.c_str(), nullptr); }

 private:
  std::string _s;

  static std::string _fmt_uint(unsigned long long v, unsigned char base) {
    if (base < 2 || base > 36) base = 10;
    if (v == 0) return "0";
    std::string out;
    while (v) {
      int d = (int)(v % base);
      out.insert(out.begin(), (char)(d < 10 ? '0' + d : 'a' + d - 10));
      v /= base;
    }
    return out;
  }
  static std::string _fmt_int(long long v, unsigned char base) {
    if (v < 0 && base == 10) return "-" + _fmt_uint((unsigned long long)(-(v + 1)) + 1, base);
    return _fmt_uint((unsigned long long)v, base);
  }
  static std::string _fmt_double(double v, unsigned int decimals) {
    char buf[64];
    snprintf(buf, sizeof(buf), "%.*f", (int)decimals, v);
    return buf;
  }
};

#endif
