#include <Arduino.h>
#include <display.h>
#include <esp_sleep.h>
#include <frames.h>
#include <fuelGauge.h>
#include <input.h>

void setup() {
  Serial.begin(115200);
  delay(100); // Short delay to ensure serial is ready

  // sleep_init(PIN_BUTTON_B);

  if (psramInit()) {
    Serial.println("PSRAM is enabled!");
    Serial.printf("PSRAM size: %u bytes\n", ESP.getPsramSize());

    display_create();
    input_init();
    fuelGauge_init();
  } else {
    Serial.println("PSRAM is not enabled or not found.");
  }
}

unsigned long last_gameUpdate = 0;
const unsigned long interval_gameUpdate = 33333; // 16.666ms (60fps)

unsigned long last_fuelGaugeUpdate = 0;
const unsigned long interval_fuelGaugeUpdate = 1000; // 1sec

int animation_index = 0;

void loop() {
  // display_power(true);
  input_read();
  input_update();

  unsigned long currentMicros = micros();
  unsigned long currentMillis = millis();
  // Serial.println(currentMicros - last_gameUpdate);

  if (currentMicros - last_gameUpdate >= interval_gameUpdate) {
    last_gameUpdate = currentMicros;
    animation_index++;

    display->drawBitmap(0, 0, animation_data[animation_index % FRAME_COUNT],
                        400, 240, 1, 0);
  }

  if (currentMillis - last_fuelGaugeUpdate >= interval_fuelGaugeUpdate) {
    last_fuelGaugeUpdate = currentMillis;
    fuelGauge_update();
  }

  fuelGauge_display();
  debug_input_display();

  display->refresh();
}
