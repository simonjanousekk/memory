#ifndef FUEL_GAUGE_H
#define FUEL_GAUGE_H

#include "Adafruit_MAX1704X.h"
#include <Arduino.h>
#include <display.h>
#include <sprites/battery.h>

Adafruit_MAX17048 fuelGauge;

float fuelGauge_voltage = 0;
float fuelGauge_percentage = 0;

int battery_levels[] = {17, 33, 50, 67, 83, 100};
const uint8_t *battery_sprite = battery_100;

void fuelGauge_init() {
  Wire.begin(4, 5);
  while (!fuelGauge.begin()) {
    Serial.println(F(
        "Couldnt find Adafruit MAX17048?\nMake sure a battery is plugged in!"));
    delay(2000);
  }
  fuelGauge.quickStart();
}

void fuelGauge_update() {
  fuelGauge_voltage = fuelGauge.cellVoltage();
  fuelGauge_percentage = fuelGauge.cellPercent();

  battery_sprite = battery_100;
  for (int i = 0; i < (sizeof(battery_levels) / sizeof(battery_levels[0])); i++) {
    if (fuelGauge_percentage < battery_levels[i]) {
      battery_sprite = battery_sprites[i];
      break;
    }
  }
}

void fuelGauge_debug_display() {
  String fuelGauge_display_text = String(int(fuelGauge_percentage)) + "% " + String(fuelGauge_voltage) + "V";
  draw_text_block(fuelGauge_display_text, 1, 1, WHITE);
}

void battery_display() {
  display.drawBitmap(SCREEN_WIDTH - 24 - 5, 5, battery_sprite, 24, 16, BLACK, WHITE);
}
#endif