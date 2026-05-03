#ifndef FUEL_GAUGE_H
#define FUEL_GAUGE_H

#include "Adafruit_MAX1704X.h"
#include <Arduino.h>
#include <display.h>
#include <sprites/battery_sprites.h>

Adafruit_MAX17048 fuel_gauge;

float fuel_gauge_voltage = 0;
float fuel_gauge_percentage = 0;

int battery_levels[] = {17, 33, 50, 67, 83, 100};
const uint8_t *battery_sprite = battery_100;

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

  battery_sprite = battery_100;
  for (int i = 0; i < (sizeof(battery_levels) / sizeof(battery_levels[0]));
       i++) {
    if (fuel_gauge_percentage < battery_levels[i]) {
      battery_sprite = battery_sprites[i];
      break;
    }
  }
}

void fuel_gauge_debug_display() {
  String text = String(int(fuel_gauge_percentage)) + "% " +
                String(fuel_gauge_voltage) + "V";
  draw_text_block(text, 1, 1, WHITE);
}

void battery_display() {
  display.drawBitmap(SCREEN_WIDTH - 24 - 5, 5, battery_sprite, 24, 16, BLACK,
                     WHITE);
}
#endif
