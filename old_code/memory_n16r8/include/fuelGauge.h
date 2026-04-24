#ifndef FUEL_GAUGE_H
#define FUEL_GAUGE_H

#include "Adafruit_MAX1704X.h"
#include <Arduino.h>
#include <display.h>

Adafruit_MAX17048 fuelGauge;

float fuelGauge_voltage = 0;
float fuelGauge_percentage = 0;

void fuelGauge_init() {
  Wire.begin(4, 5);
  while (!Serial) {
    delay(10);
  }
  while (!fuelGauge.begin()) {
    Serial.println(F(
        "Couldnt find Adafruit MAX17048?\nMake sure a battery is plugged in!"));
    delay(2000);
  }
  // fuelGauge.quickStart();
  Serial.print(F("Found MAX17048"));
  Serial.print(F(" with Chip ID: 0x"));
  Serial.println(fuelGauge.getChipID(), HEX);
}

void fuelGauge_update() {
  fuelGauge_voltage = fuelGauge.cellVoltage();
  fuelGauge_percentage = fuelGauge.cellPercent();
}

void fuelGauge_display() {
  int16_t x1, y1;
  uint16_t w, h;

  String fuelGauge_display_text =
      String(fuelGauge_percentage) + "% " + String(fuelGauge_voltage) + "V";
  display->getTextBounds(fuelGauge_display_text, 0, 0, &x1, &y1, &w, &h);

  display->setTextSize(2);
  display->fillRect(x1, y1, w, h, BLACK);

  display->setCursor(0, 0);
  display->setTextColor(WHITE);
  display->print(fuelGauge_display_text);
}
#endif
