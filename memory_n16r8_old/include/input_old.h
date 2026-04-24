#ifndef INPUT_H
#define INPUT_H

#include <Arduino.h>
#include <ESP32Encoder.h>
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

ESP32Encoder encoder;

// State variables
float encoder_value = 0;
float last_encoder_value = 0;
bool button_a_state, button_b_state, switch_state, encoder_button_state = false;
bool last_button_a_state, last_button_b_state, last_switch_state, last_encoder_button_state = false;
unsigned long encoder_button_last_pressed = ULONG_MAX;
// Misc variables
const unsigned long long_press_duration = 1000;
float debug_rotation = -PI / 2;

void input_init() {
  encoder.attachHalfQuad(PIN_ENCODER_A, PIN_ENCODER_B);
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
  encoder_value = encoder.getCount();
}

void input_update() {
  if (switch_state != last_switch_state) { // SWITCH
    if (switch_state == LOW) {
      sleep_sleep(PIN_SWITCH);
    }
    last_switch_state = switch_state;
  }
  if (button_a_state != last_button_a_state) { // BUTTON A
    if (button_a_state == LOW) {
    }
    last_button_a_state = button_a_state;
  }
  if (button_b_state != last_button_b_state) { // BUTTON B
    if (button_b_state == LOW) {
    }
    last_button_b_state = button_b_state;
  }
  if (encoder_button_state != last_encoder_button_state) { // ENCODER BUTTON
    if (encoder_button_state == LOW) {
      encoder_button_last_pressed = millis();
    } else {
      encoder_button_last_pressed = ULONG_MAX;
    }
    last_encoder_button_state = encoder_button_state;
  } else if (millis() - encoder_button_last_pressed > long_press_duration) {
    // LONG PRESS
    Serial.println("LONG PRESS");
    encoder_button_last_pressed = ULONG_MAX;
  }
  if (encoder_value != last_encoder_value) { // ENCODER
    debug_rotation -= float(encoder_value - last_encoder_value) * 0.2094395102f;
    last_encoder_value = encoder_value;
  }
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
  debug_input_button(25, 50, button_a_state);
  debug_input_button(25, SCREEN_HEIGHT - 50, button_b_state);

  // SWITCH
  debug_input_button(SCREEN_WIDTH - 25, 25, switch_state);

  // ENCODER
  debug_input_button(SCREEN_WIDTH - 25, SCREEN_HEIGHT / 2, encoder_button_state);
  display.fillCircle(
      SCREEN_WIDTH - 25 + cos(debug_rotation) * 20,
      SCREEN_HEIGHT / 2 + sin(debug_rotation) * 20,
      5,
      BLACK);
}

#endif