// Arduino / ESP32 core functions for the native simulator: time, pins,
// random numbers, tasks and the global Serial / SPI / Wire objects.

#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>

#include <atomic>
#include <chrono>
#include <mutex>
#include <random>
#include <thread>

#include "sim.h"

HardwareSerial Serial;
SPIClass SPI;
TwoWire Wire;

// ---------------------------------------------------------------------------
// Time
// ---------------------------------------------------------------------------
static std::chrono::steady_clock::time_point _boot_time() {
  static const auto t0 = std::chrono::steady_clock::now();
  return t0;
}

unsigned long millis() {
  return (unsigned long)std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now() - _boot_time())
      .count();
}

unsigned long micros() {
  return (unsigned long)std::chrono::duration_cast<std::chrono::microseconds>(
      std::chrono::steady_clock::now() - _boot_time())
      .count();
}

// On the main thread, keep the window responsive while blocking.
void delay(uint32_t ms) {
  if (!sim_on_main_thread()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    return;
  }
  const auto end = std::chrono::steady_clock::now() + std::chrono::milliseconds(ms);
  while (std::chrono::steady_clock::now() < end) {
    sim_pump_events();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
}

void delayMicroseconds(uint32_t us) {
  std::this_thread::sleep_for(std::chrono::microseconds(us));
}

void yield() {
  if (sim_on_main_thread()) sim_pump_events();
}

// ---------------------------------------------------------------------------
// Pins. Inputs idle HIGH (all buttons are active-low with pull-ups).
// ---------------------------------------------------------------------------
static std::atomic<uint8_t> _pins[64];

static struct _PinInit {
  _PinInit() {
    for (auto& p : _pins) p = HIGH;
  }
} _pin_init;

void pinMode(uint8_t /*pin*/, uint8_t /*mode*/) {}

void digitalWrite(uint8_t pin, uint8_t val) {
  if (pin >= 64) return;
  _pins[pin] = val ? HIGH : LOW;
  sim_lcd_pin(pin, val ? HIGH : LOW);
}

int digitalRead(uint8_t pin) {
  return pin < 64 ? _pins[pin].load() : LOW;
}

void sim_set_pin(uint8_t pin, uint8_t level) {
  if (pin < 64) _pins[pin] = level;
}

// ---------------------------------------------------------------------------
// Random — same semantics as arduino-esp32 WMath.cpp. After randomSeed(),
// random() uses newlib's rand() LCG, reproduced here so a seed generates the
// same maze / words / count layout as on the device.
// ---------------------------------------------------------------------------
static bool _use_random_hw = true;
static uint64_t _rand_next = 1;

static uint32_t _newlib_rand() {
  _rand_next = _rand_next * 6364136223846793005ULL + 1;
  return (uint32_t)((_rand_next >> 32) & 0x7fffffff);
}

uint32_t esp_random() {
  static std::random_device rd;
  static std::mutex m;
  std::lock_guard<std::mutex> lock(m);
  return rd();
}

void randomSeed(unsigned long seed) {
  if (seed != 0) {
    _rand_next = (uint32_t)seed;  // newlib srand() takes an unsigned int
    _use_random_hw = false;
  }
}

long random(long howbig) {
  if (howbig == 0) return 0;
  if (howbig < 0) return random(0, -howbig);
  uint32_t val = _use_random_hw ? esp_random() : _newlib_rand();
  return (long)(val % (uint32_t)howbig);
}

long random(long howsmall, long howbig) {
  if (howsmall >= howbig) return howsmall;
  long diff = howbig - howsmall;
  return random(diff) + howsmall;
}

long map(long x, long in_min, long in_max, long out_min, long out_max) {
  const long run = in_max - in_min;
  if (run == 0) return -1;
  const long rise = out_max - out_min;
  const long delta = x - in_min;
  return (delta * rise) / run + out_min;
}

// ---------------------------------------------------------------------------
// FreeRTOS
// ---------------------------------------------------------------------------
BaseType_t xTaskCreate(TaskFunction_t fn, const char* /*name*/, uint32_t /*stack_depth*/,
    void* param, UBaseType_t /*priority*/, TaskHandle_t* handle) {
  std::thread(fn, param).detach();
  if (handle) *handle = nullptr;
  return pdPASS;
}
