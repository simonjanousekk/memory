#ifndef TEST_SCREEN_H
#define TEST_SCREEN_H

#include <Arduino.h>
#include <display.h>
#include <screen.h>

class TestScreen : public Screen {
  bool _done = false;

 public:
  ScreenMode id() const override { return SCREEN_TEST; }

  void draw() override {
    display.setFont(&Panell_Regular12pt7b);
    display.setTextColor(BLACK);
    display.setTextSize(1);
    display.setCursor(10, 10);
    display.println("Test Screen");

    display.setFont(&Panell_Extended24pt7b);
    display.setTextColor(BLACK);
    display.setTextSize(1);
    display.setCursor(10, 200);
    display.println("Test Screen");

    display.setFont(&FreeMonoBold9pt7b);
  }
};

#endif