#ifndef FUEL_GAUGE_H
#define FUEL_GAUGE_H

#include <Arduino.h>
#include <display.h>
#include <sprites/battery_sprites.h>

#include "Adafruit_MAX1704X.h"

Adafruit_MAX17048 fuel_gauge;

float fuel_gauge_voltage = 0;
float fuel_gauge_percentage = 0;

int battery_levels[] = {17, 33, 50, 67, 83, 100};
const uint8_t* battery_sprite = battery_100;

int fuel_gauge_percentage_int() {
  return int(fuel_gauge_percentage);
}

void fuel_gauge_init() {
  Wire.begin(4, 5);
  while (!fuel_gauge.begin()) {
    Serial.println(F(
        "Couldnt find Adafruit MAX17048?\nMake sure a battery is plugged in!"));
    delay(2000);
  }
  fuel_gauge.quickStart();
}

void fuel_gauge_update() {
  fuel_gauge_voltage = fuel_gauge.cellVoltage();
  fuel_gauge_percentage = fuel_gauge.cellPercent();
  Serial.printf("Voltage: %.2fV, Percentage: %d%%\n", fuel_gauge_voltage,
      fuel_gauge_percentage_int());

  battery_sprite = battery_100;
  for (int i = 0; i < (sizeof(battery_levels) / sizeof(battery_levels[0]));
      i++) {
    if (fuel_gauge_percentage < battery_levels[i]) {
      battery_sprite = battery_sprites[i];
      break;
    }
  }
}

void fuel_gauge_debug_display(char* buf, int len) {
  snprintf(buf, len, "%d%% %.2fV", fuel_gauge_percentage_int(),
      fuel_gauge_voltage);
}

void battery_display() {
  display.drawBitmap(SCREEN_WIDTH - 24 - 5, 5, battery_sprite, 24, 16, BLACK,
      WHITE);
}
#endif
