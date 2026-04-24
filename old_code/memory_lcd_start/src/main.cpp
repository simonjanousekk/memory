#include <Adafruit_GFX.h>
#include <Adafruit_SharpMem.h>
#include <ESP32Encoder.h>

// #include "battery_monitor.h"
#include "frames.h"
// #include "grid.h"
#include "sleep_manager.h"

// ---- Pin configuration (centralized) ----
// Display (SHARP LCD)

#define PIN_DISPLAY_SCK 12
#define PIN_DISPLAY_MOSI 11
#define PIN_DISPLAY_SS 10
#define PIN_DISPLAY_POWER 38 // Display power/enable

#define SCREEN_WIDTH 400
#define SCREEN_HEIGHT 240

// Encoder
#define PIN_ENCODER_A 17
#define PIN_ENCODER_B 18
#define PIN_ENCODER_BUTTON 8

#define PIN_SWITCH 15
#define PIN_BUTTON_A 6
#define PIN_BUTTON_B 7

#define PIN_GAUGE_ALERT 16

// Battery monitor (Adafruit HUZZAH32: VBAT on A13/GPIO35)
// #define PIN_VBAT 35

// randomSeed uses analog pin (26 = A0 on Feather)
#define PIN_RANDOM_SEED 26
bool encoderButtonState = false;
int encoderValue = 0;
ESP32Encoder encoder;

Adafruit_SharpMem display(PIN_DISPLAY_SCK, PIN_DISPLAY_MOSI, PIN_DISPLAY_SS,
                          SCREEN_WIDTH, SCREEN_HEIGHT);

// noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);

// const int bouncingBallsCount = 50;

// bouncingBall bouncingBalls[bouncingBallsCount];

void setup(void) {
  // HELLO
  Serial.begin(9600);
  Serial.println("Hello World");

  // RANDOM SEED
  float seed = analogRead(PIN_RANDOM_SEED);
  Serial.println("random seed: " + String(seed));
  randomSeed(seed);

  // INIT BACKGROUND PROCESSES
  initSleep(PIN_DISPLAY_POWER, PIN_SWITCH, display);
  // initBatteryMonitor(PIN_VBAT);

  // INIT ENCODER
  pinMode(PIN_ENCODER_A, INPUT_PULLUP);
  pinMode(PIN_ENCODER_B, INPUT_PULLUP);
  encoder.attachHalfQuad(PIN_ENCODER_A, PIN_ENCODER_B);
  encoder.setCount(encoderValue);

  // INIT BUTTONS AND SWITCHES
  // pinMode(PIN_SWITCH, INPUT_PULLUP); // inited in initSleep (kinda stupid)
  pinMode(PIN_BUTTON_A, INPUT_PULLUP);
  pinMode(PIN_BUTTON_B, INPUT_PULLUP);

  // INIT DISPLAY
  display.begin();
  display.setRotation(0);
  display.clearDisplay();

  // initBuffers();

  // Initialize bouncing balls
  // int radius = 10;
  // for (int i = 0; i < bouncingBallsCount; i++) {
  //   bouncingBalls[i] =
  //       bouncingBall(random(radius, SCREEN_WIDTH - radius),
  //                    random(radius, SCREEN_HEIGHT - radius), int(random(-2,
  //                    2)), int(random(-2, 2)), radius);
  // }
}

// int x = 200;
// int y = 120;
// bool changingX = true;
float animationIndex = 0;

int frameCount = 0;

void loop(void) {

  // Check button for sleep
  checkButtonForSleep(display);

  // currentBuffer.fillScreen(1);
  display.clearDisplayBuffer();

  // for (int i = 0; i < bouncingBallsCount; i++) {
  //   bouncingBalls[i].update(bouncingBalls);
  //   bouncingBalls[i].draw(display);
  // }

  // bool currentButtonState = digitalRead(PIN_ENCODER_BUTTON);
  // if (currentButtonState == LOW && encoderButtonState == HIGH) {
  //   changingX = !changingX;
  // }
  // encoderButtonState = currentButtonState;

  // if (encoder.getCount() != encoderValue) {
  //   int diff = encoder.getCount() - encoderValue;
  //   // Add acceleration: each full step (multiple of 2) adds an extra
  //   // multiplier to the step
  //   int step = (diff > 0) ? 1 : -1;
  //   int absDiff = abs(diff);
  //   int accel = 0;
  //   int lastDelta = 0;
  //   for (int i = 1; i <= absDiff; i++) {
  //     accel += i;
  //     lastDelta = step * accel;
  //   }
  //   if (changingX) {
  //     x += step * accel;
  //     if (x < size / 2) {
  //       x = size / 2;
  //     }
  //     if (x > SCREEN_WIDTH - size / 2) {
  //       x = SCREEN_WIDTH - size / 2;
  //     }
  //   } else {
  //     y += step * accel;
  //     encoderValue = encoder.getCount();
  //     if (y < size / 2) {
  //       y = size / 2;
  //     }
  //     if (y > SCREEN_HEIGHT - size / 2) {
  //       y = SCREEN_HEIGHT - size / 2;
  //     }
  //   }
  //   Serial.println(step * accel);
  //   encoderValue = encoder.getCount();
  // }

  // display.drawFastHLine(0, y, SCREEN_WIDTH, 0);
  // display.drawFastVLine(x, 0, SCREEN_HEIGHT, 0);

  // display.fillRect(x - size / 2, y - size / 2, size, size, 0);
  // display.fillRect(x - size / 2 + strokeWidth, y - size / 2 + strokeWidth,
  //                  size - strokeWidth * 2, size - strokeWidth * 2, 1);
  // if (frameCount % 4 == 0) {
  //   if (animationIndex < epd_bitmap_BladeVamp_outlineArray_LEN - 1) {
  //     animationIndex++;
  //   } else {
  //     animationIndex = 0;
  //   }
  // }
  // Serial.println(animationIndex);
  // display.drawBitmap(x, y, epd_bitmap_BladeVamp_outlineArray[animationIndex],
  //                    32, 32, 1, 0);

  animationIndex += (encoder.getCount() - encoderValue) / 2.0f;
  encoderValue = encoder.getCount();
  if (animationIndex < 0) {
    animationIndex = FRAME_COUNT - 1;
  }
  if (animationIndex >= FRAME_COUNT) {
    animationIndex = 0;
  }

  // display.drawBitmap(0, 0, animation_data[int(animationIndex)], 400, 240, 1,
  // 0);
  display.drawBitmap(0, 0, animation_data[frameCount % FRAME_COUNT], 400, 240,
                     1, 0);

  // if (frameCount % 2 == 0) {
  //   if (animationIndex < FRAME_COUNT - 1) {
  //     animationIndex++;
  //   } else {
  //     animationIndex = 0;
  //   }
  // }

  // updateBatteryStatus();
  // drawBatteryStatus(display);

  display.refresh();
  frameCount++;
}
