#ifndef SIM_PRINT_H
#define SIM_PRINT_H

// Arduino Print base class for the native simulator. Adafruit_GFX derives
// from it; Serial uses it to write to stdout.

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "WString.h"

#define DEC 10
#define HEX 16
#define OCT 8
#define BIN 2

class Print {
 public:
  virtual ~Print() {}
  virtual size_t write(uint8_t c) = 0;
  virtual size_t write(const uint8_t* buf, size_t n) {
    size_t r = 0;
    while (n--) r += write(*buf++);
    return r;
  }
  size_t write(const char* s) { return s ? write((const uint8_t*)s, strlen(s)) : 0; }
  size_t write(const char* buf, size_t n) { return write((const uint8_t*)buf, n); }
  virtual void flush() {}

  size_t print(const char* s) { return write(s); }
  size_t print(const String& s) { return write(s.c_str(), s.length()); }
  size_t print(char c) { return write((uint8_t)c); }
  size_t print(int v, int base = DEC) { return print(String(v, (unsigned char)base)); }
  size_t print(unsigned int v, int base = DEC) { return print(String(v, (unsigned char)base)); }
  size_t print(long v, int base = DEC) { return print(String(v, (unsigned char)base)); }
  size_t print(unsigned long v, int base = DEC) { return print(String(v, (unsigned char)base)); }
  size_t print(long long v, int base = DEC) { return print(String(v, (unsigned char)base)); }
  size_t print(unsigned long long v, int base = DEC) { return print(String(v, (unsigned char)base)); }
  size_t print(double v, int digits = 2) { return print(String(v, (unsigned int)digits)); }

  size_t println() { return write("\r\n"); }
  template <typename T>
  size_t println(const T& v) {
    size_t n = print(v);
    return n + println();
  }
  template <typename T>
  size_t println(const T& v, int fmt) {
    size_t n = print(v, fmt);
    return n + println();
  }

  size_t printf(const char* fmt, ...) __attribute__((format(printf, 2, 3))) {
    char small[128];
    va_list ap;
    va_start(ap, fmt);
    int len = vsnprintf(small, sizeof(small), fmt, ap);
    va_end(ap);
    if (len < 0) return 0;
    if ((size_t)len < sizeof(small)) return write(small, (size_t)len);
    char* big = new char[len + 1];
    va_start(ap, fmt);
    vsnprintf(big, len + 1, fmt, ap);
    va_end(ap);
    size_t n = write(big, (size_t)len);
    delete[] big;
    return n;
  }
};

#endif
