#ifndef SLEEP_MANAGER_H
#define SLEEP_MANAGER_H

#include <Arduino.h>
#include <display.h>
#include <input.h>

void sleep_init(int wake_pin) { pinMode(wake_pin, INPUT); }

void sleep_sleep(int wake_pin) {
  Serial.println("Going to sleep");
  // Turn off display before sleep
  display_power(false);

  // Configure the wakeup source: RTC GPIO (EXT0) on wake_pin, low level
  esp_sleep_enable_ext0_wakeup((gpio_num_t)wake_pin, 1);

  // Go to sleep
  esp_deep_sleep_start();
}

#endif