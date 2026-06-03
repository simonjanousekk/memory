#ifndef INPUT_H
#define INPUT_H

#include <Arduino.h>
#include <ESP32Encoder.h>
#include <OneButton.h>
#include <display.h>
#include <screen.h>
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

OneButton button_a, button_b, button_p, button_e;

float encoder_value = 0;
float last_encoder_value = 0;
float debug_rotation = -PI / 2;

static bool _ab_combo_fired = false;

void input_init() {
  button_a.setup(PIN_BUTTON_A, INPUT, true);
  button_b.setup(PIN_BUTTON_B, INPUT, true);
  button_p.setup(PIN_BUTTON_P, INPUT, true);
  button_e.setup(PIN_ENCODER_BUTTON, INPUT, true);

  button_a.setDebounceMs(0);
  button_b.setDebounceMs(0);
  button_p.setDebounceMs(0);
  button_e.setDebounceMs(0);

  // Counts in hardware/ISR; main loop only reads getCount() and dispatches.
  encoder.attachHalfQuad(PIN_ENCODER_A, PIN_ENCODER_B);

  button_p.attachPress([]() { sleep_sleep(PIN_BUTTON_P); });

  // A+B held → toggle debug menu, returning to the screen you came from.
  // Guard flag prevents both long-press callbacks firing and toggling back.
  button_a.attachLongPressStart([]() {
    if (!_ab_combo_fired && digitalRead(PIN_BUTTON_B) == LOW) {
      _ab_combo_fired = true;
      toggle_debug_menu();
    }
  });
  button_b.attachLongPressStart([]() {
    if (!_ab_combo_fired && digitalRead(PIN_BUTTON_A) == LOW) {
      _ab_combo_fired = true;
      toggle_debug_menu();
    }
  });

  button_a.attachPress([]() {
    if (screen_input_allowed()) currentScreen->on_button_a();
  });
  button_b.attachPress([]() {
    if (screen_input_allowed()) currentScreen->on_button_b();
  });

  button_e.attachPress([]() {
    if (screen_input_allowed()) currentScreen->on_encoder_press();
  });
  // button_e.attachLongPressStart([]() { cycle_screen(); });
  // button_e.attachPress([]() { cycle_screen(); });
}

void input_update() {
  button_a.tick();
  button_b.tick();
  button_p.tick();
  button_e.tick();

  if (digitalRead(PIN_BUTTON_A) != LOW && digitalRead(PIN_BUTTON_B) != LOW)
    _ab_combo_fired = false;

  encoder_value = encoder.getCount();
  int encoder_delta = int(encoder_value - last_encoder_value);
  debug_rotation -= float(encoder_delta) * 0.2094395102f;
  if (encoder_delta != 0 && screen_input_allowed())
    currentScreen->on_encoder_rotate(-encoder_delta);
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

void debug_input_display(char * /*buf*/, int /*len*/) {
  // Graphics overlay — draws directly, buffer intentionally left empty.
  debug_input_button(25, 50, button_a.debouncedValue() == 0);
  debug_input_button(25, SCREEN_HEIGHT - 50, button_b.debouncedValue() == 0);

  debug_input_button(SCREEN_WIDTH - 25, SCREEN_HEIGHT / 2,
                     button_e.debouncedValue() == 0);

  display.fillCircle(SCREEN_WIDTH - 25 + cos(debug_rotation) * 20,
                     SCREEN_HEIGHT / 2 + sin(debug_rotation) * 20, 5, BLACK);
}

#endif
