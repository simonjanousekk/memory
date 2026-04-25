#ifndef INPUT_H
#define INPUT_H

#include <Arduino.h>
#include <ESP32Encoder.h>
#include <OneButton.h>
#include <debug_menu.h>
#include <display.h>
#include <sleep_manager.h>

// ENCODER DECLARATIONS
#define PIN_ENCODER_A 17
#define PIN_ENCODER_B 18
#define PIN_ENCODER_BUTTON 8

// BUTTON DECLARATIONS
#define PIN_BUTTON_P 15
#define PIN_BUTTON_A 6
#define PIN_BUTTON_B 7

ESP32Encoder encoder;

OneButton buttonA, buttonB, buttonP, buttonE;

float encoder_value = 0;
float last_encoder_value = 0;
float debug_rotation = -PI / 2;

//TMP
bool zajac = false;

void input_init() {
  buttonA.setup(PIN_BUTTON_A, INPUT, true);
  buttonB.setup(PIN_BUTTON_B, INPUT, true);
  buttonP.setup(PIN_BUTTON_P, INPUT, true);
  buttonE.setup(PIN_ENCODER_BUTTON, INPUT, true);

  buttonA.setDebounceMs(0);
  buttonB.setDebounceMs(0);
  buttonP.setDebounceMs(0);
  buttonE.setDebounceMs(0);

  encoder.attachHalfQuad(PIN_ENCODER_A, PIN_ENCODER_B);

  buttonP.attachPress([]() { sleep_sleep(PIN_BUTTON_P); });
  buttonA.attachPress([]() {
    if (debug_menu_is_open()) {
      debug_menu_confirm_selected();
    } else {
      zajac = !zajac;
    }
  });
  buttonB.attachPress([]() {
    if (debug_menu_is_open()) {
      debug_menu_exit();
    }
  });
  buttonE.attachPress([]() {
    if (debug_menu_is_open()) {
      debug_menu_confirm_selected();
    }
  });
  buttonE.attachLongPressStart([]() { debug_menu_enter_or_exit(); });
}

void input_update() {
  buttonA.tick();
  buttonB.tick();
  buttonP.tick();
  buttonE.tick();

  encoder_value = encoder.getCount();
  int encoder_delta = int(encoder_value - last_encoder_value);
  debug_rotation -= float(encoder_delta) * 0.2094395102f;
  debug_menu_encoder_step(-encoder_delta);
  last_encoder_value = encoder_value;
}

void debug_input_button(int x, int y, bool state) {
  if (!state) {
    display.fillCircle(x, y, 10, BLACK);
  } else {
    display.fillCircle(x, y, 10, BLACK);
    display.fillCircle(x, y, 7, WHITE);
  }
}

void debug_input_display() {
  // BUTTONS
  debug_input_button(25, 50, buttonA.debouncedValue() == 0);
  debug_input_button(25, SCREEN_HEIGHT - 50, buttonB.debouncedValue() == 0);

  // ENCODER
  debug_input_button(SCREEN_WIDTH - 25, SCREEN_HEIGHT / 2, buttonE.debouncedValue() == 0);

  display.fillCircle(
      SCREEN_WIDTH - 25 + cos(debug_rotation) * 20,
      SCREEN_HEIGHT / 2 + sin(debug_rotation) * 20,
      5, BLACK);
}

#endif