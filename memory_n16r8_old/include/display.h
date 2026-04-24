#ifndef DISPLAY_H
#define DISPLAY_H

#include <Adafruit_SharpMem.h>

// DISPLAY DECLARATIONS
#define SCREEN_WIDTH 400
#define SCREEN_HEIGHT 240

#define PIN_DISPLAY_SCK 12
#define PIN_DISPLAY_MOSI 11
#define PIN_DISPLAY_SS 10
#define PIN_DISPLAY_ON 38

#define WHITE 1
#define BLACK 0

Adafruit_SharpMem display(PIN_DISPLAY_SCK, PIN_DISPLAY_MOSI, PIN_DISPLAY_SS, SCREEN_WIDTH, SCREEN_HEIGHT);

void display_init() {
  display.begin();
  pinMode(PIN_DISPLAY_ON, OUTPUT);
  digitalWrite(PIN_DISPLAY_ON, true);
}

void display_power(bool power) {
  // Serial.println("Display power: " + String(power));
  display.clearDisplayBuffer();
  display.refresh();
  digitalWrite(PIN_DISPLAY_ON, power ? HIGH : LOW);
}

#endif