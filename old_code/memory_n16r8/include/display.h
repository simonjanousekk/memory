#ifndef DISPLAY_H
#define DISPLAY_H

#include <Adafruit_SharpMem.h>
#include <Arduino.h>

// DISPLAY DECLARATIONS
#define SCREEN_WIDTH 400
#define SCREEN_HEIGHT 240

#define PIN_DISPLAY_SCK 12
#define PIN_DISPLAY_MOSI 11
#define PIN_DISPLAY_SS 10
#define PIN_DISPLAY_ON 38

#define WHITE 0
#define BLACK 1

Adafruit_SharpMem *display = nullptr;

bool display_create() {
  display = new Adafruit_SharpMem(PIN_DISPLAY_SCK, PIN_DISPLAY_MOSI,
                                  PIN_DISPLAY_SS, SCREEN_WIDTH, SCREEN_HEIGHT);
  display->begin();
  pinMode(PIN_DISPLAY_ON, OUTPUT);
  digitalWrite(PIN_DISPLAY_ON, HIGH);
  return true;
}
// Adafruit_SharpMem display(PIN_DISPLAY_SCK, PIN_DISPLAY_MOSI, PIN_DISPLAY_SS,
//                           SCREEN_WIDTH, SCREEN_HEIGHT);

// void display_init() {
//   display.begin();
//   pinMode(PIN_DISPLAY_ON, OUTPUT);
//   digitalWrite(PIN_DISPLAY_ON, HIGH);
// }

void display_power(bool power) {
  if (power) {
    digitalWrite(PIN_DISPLAY_ON, HIGH);
  } else {
    display->clearDisplay();
    digitalWrite(PIN_DISPLAY_ON, LOW);
  }
}

#endif