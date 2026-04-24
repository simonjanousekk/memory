#ifndef SLEEP_MANAGER_H
#define SLEEP_MANAGER_H

#include <Arduino.h>
#include <display.h>

void sleep_sleep(int wakepin) {

  display_power(false);

  gpio_num_t wakeGpio = static_cast<gpio_num_t>(wakepin);
  esp_sleep_enable_ext0_wakeup(wakeGpio, 1); // wake when pin == HIGH

  delay(50);
  esp_deep_sleep_start();
}

void sleep_init() {
}

#endif