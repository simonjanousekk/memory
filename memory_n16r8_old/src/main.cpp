#include <Arduino.h>
#include <display.h>
#include <fuelGauge.h>
#include <input.h>
#include <sleep_manager.h>
#include <sprites/bob.h>
#include <sprites/zajac.h>

void setup() {
  Serial.begin(115200);
  delay(100); // Short delay to ensure serial is ready

  display_init();
  input_init();
  sleep_init();
  fuelGauge_init();
}

int animation_index = 0;
unsigned long last_gameUpdate = 0;
const unsigned long interval_gameUpdate = 83333; // 16.666ms (60fps)

unsigned long last_fgUpdate = 0;
const unsigned long interval_fgUpdate = 1000; // 1 second

void loop() {
  input_update();

  unsigned long currentMicros = micros();
  unsigned long currentMillis = millis();

  if (currentMicros - last_gameUpdate >= interval_gameUpdate) {
    last_gameUpdate = currentMicros;
    animation_index++;
  }

  display.clearDisplayBuffer();
  display.drawBitmap(
      SCREEN_WIDTH / 2 - ZAJAC_WIDTH / 2,
      SCREEN_HEIGHT / 2 - ZAJAC_HEIGHT / 2,
      zajac_data[animation_index % ZAJAC_FRAME_COUNT],
      ZAJAC_WIDTH,
      ZAJAC_HEIGHT,
      0);

  if (currentMillis - last_fgUpdate >= interval_fgUpdate) {
    last_fgUpdate = currentMillis;
    fuelGauge_update();
  }

  fuelGauge_display();
  debug_input_display();
  display.refresh();
}