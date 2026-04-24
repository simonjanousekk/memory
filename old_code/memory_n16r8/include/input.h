#ifndef INPUT_H
#define INPUT_H

#include "esp32-hal-gpio.h"
#include <Arduino.h>
// #include <ESP32Encoder.h>
#include <display.h>
#include <sleep_manager.h>

// ENCODER DECLARATIONS
#define PIN_ENCODER_A 17
#define PIN_ENCODER_B 18
#define PIN_ENCODER_BUTTON 8

// BUTTON DECLARATIONS
#define PIN_SWITCH 15
#define PIN_BUTTON_A 6
#define PIN_BUTTON_B 7

// ESP32Encoder encoder;

float encoder_value = 0;
float last_encoder_value = 0;
bool button_a_state, button_b_state, switch_state, encoder_button_state = false;
bool last_button_a_state, last_button_b_state, last_switch_state,
    last_encoder_button_state = false;

void input_init() {
  // encoder.attachFullQuad(PIN_ENCODER_A, PIN_ENCODER_B);
  pinMode(PIN_ENCODER_BUTTON, INPUT);
  pinMode(PIN_SWITCH, INPUT);
  pinMode(PIN_BUTTON_A, INPUT);
  pinMode(PIN_BUTTON_B, INPUT);
}

void input_read() {
  button_a_state = digitalRead(PIN_BUTTON_A);
  button_b_state = digitalRead(PIN_BUTTON_B);
  switch_state = digitalRead(PIN_SWITCH);
  encoder_button_state = digitalRead(PIN_ENCODER_BUTTON);
  // encoder_value = encoder.getCount() / 2.0f;
}

void input_update() {
  if (switch_state != last_switch_state) {
    last_switch_state = switch_state;
    if (switch_state == LOW) {
      sleep_sleep(PIN_SWITCH);
    }
  }
  if (button_a_state != last_button_a_state) {
    last_button_a_state = button_a_state;
    if (button_a_state == LOW) {
      //
    }
  }
  if (button_b_state != last_button_b_state) {
    last_button_b_state = button_b_state;
    if (button_b_state == LOW) {
      //
    }
  }
  if (encoder_button_state != last_encoder_button_state) {
    last_encoder_button_state = encoder_button_state;
    if (encoder_button_state == LOW) {
      //
    }
  }
}

void debug_input_display() {
  display->fillCircle(25, 50, 10, button_a_state ? WHITE : BLACK);
  display->fillCircle(25, SCREEN_HEIGHT - 50, 10,
                      button_b_state ? WHITE : BLACK);

  // display->fillCircle(SCREEN_WIDTH - 25, 25, 10, switch_state ? WHITE :
  // BLACK);
}
#endif
