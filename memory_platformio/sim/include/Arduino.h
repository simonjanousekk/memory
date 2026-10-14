#ifndef SIM_ARDUINO_H
#define SIM_ARDUINO_H

// Arduino / ESP32 core shim for the native simulator (env:native).
// Covers only what the firmware and its libraries use. Hardware is emulated
// at the pin / SPI level in sim/src, so firmware code runs unchanged.

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <algorithm>
#include <cmath>

#include "Print.h"
#include "WString.h"

typedef bool boolean;
typedef uint8_t byte;

// ---------------------------------------------------------------------------
// Constants and macros
// ---------------------------------------------------------------------------
#define HIGH 0x1
#define LOW 0x0

#define INPUT 0x01
#define OUTPUT 0x03
#define PULLUP 0x04
#define INPUT_PULLUP 0x05
#define PULLDOWN 0x08
#define INPUT_PULLDOWN 0x09

#define LSBFIRST 0
#define MSBFIRST 1

#define PI 3.1415926535897932384626433832795
#define HALF_PI 1.5707963267948966192313216916398
#define TWO_PI 6.283185307179586476925286766559
#define DEG_TO_RAD 0.017453292519943295769236907684886
#define RAD_TO_DEG 57.295779513082320876798154814105

#define PROGMEM
#define F(s) (s)
#define pgm_read_byte(addr) (*(const uint8_t*)(addr))
#define pgm_read_word(addr) (*(const uint16_t*)(addr))
// pgm_read_dword is left to Adafruit GFX, which uses it to read pointers.

using std::abs;
using std::max;
using std::min;

#define constrain(amt, low, high) ((amt) < (low) ? (low) : ((amt) > (high) ? (high) : (amt)))
#define radians(deg) ((deg) * DEG_TO_RAD)
#define degrees(rad) ((rad) * RAD_TO_DEG)
#define sq(x) ((x) * (x))

// ---------------------------------------------------------------------------
// Time, pins, random (sim/src/arduino_core.cpp)
// ---------------------------------------------------------------------------
unsigned long millis();
unsigned long micros();
void delay(uint32_t ms);
void delayMicroseconds(uint32_t us);
void yield();

void pinMode(uint8_t pin, uint8_t mode);
void digitalWrite(uint8_t pin, uint8_t val);
int digitalRead(uint8_t pin);

uint32_t esp_random();
void randomSeed(unsigned long seed);
long random(long howbig);
long random(long howsmall, long howbig);
long map(long x, long in_min, long in_max, long out_min, long out_max);

// ---------------------------------------------------------------------------
// Serial → stdout
// ---------------------------------------------------------------------------
class HardwareSerial : public Print {
 public:
  void begin(unsigned long /*baud*/) {}
  using Print::write;
  size_t write(uint8_t c) override { return fputc(c, stdout) == EOF ? 0 : 1; }
  size_t write(const uint8_t* buf, size_t n) override { return fwrite(buf, 1, n, stdout); }
  void flush() override { fflush(stdout); }
  operator bool() const { return true; }
};
extern HardwareSerial Serial;

// ---------------------------------------------------------------------------
// FreeRTOS tasks → std::thread (detached)
// ---------------------------------------------------------------------------
typedef void (*TaskFunction_t)(void*);
typedef void* TaskHandle_t;
typedef int BaseType_t;
typedef unsigned int UBaseType_t;
typedef uint32_t TickType_t;

#define pdPASS 1
#define pdFAIL 0
#define portTICK_PERIOD_MS 1
#define pdMS_TO_TICKS(ms) ((TickType_t)(ms))

BaseType_t xTaskCreate(TaskFunction_t fn, const char* name, uint32_t stack_depth,
    void* param, UBaseType_t priority, TaskHandle_t* handle);
// Only vTaskDelete(NULL) as the last statement of a task is supported: the
// thread then simply returns.
inline void vTaskDelete(TaskHandle_t /*task*/) {}
inline void vTaskDelay(TickType_t ticks) { delay(ticks); }

// ---------------------------------------------------------------------------
// Deep sleep (sim/src/sim_main.cpp)
// ---------------------------------------------------------------------------
enum gpio_num_t : int { GPIO_NUM_NC = -1 };
typedef int esp_err_t;
#define ESP_OK 0

esp_err_t esp_sleep_enable_ext0_wakeup(gpio_num_t gpio, int level);
[[noreturn]] void esp_deep_sleep_start();

// ---------------------------------------------------------------------------
// Sketch entry points (src/main.cpp)
// ---------------------------------------------------------------------------
void setup();
void loop();

#endif
