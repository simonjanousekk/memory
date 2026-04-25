#include <Arduino.h>
#include <debug_menu.h>
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
  fuelGauge_update();
}

uint16_t animation_index = 0;
unsigned long last_gameUpdate = 0;
const unsigned long interval_gameUpdate = 1000000 / 12; //83333; // 16.666ms (60fps)

unsigned long last_fgUpdate = -9000;
const unsigned long interval_fgUpdate = 10000; // 10 seconds

void loop() {
  input_update();

  unsigned long currentMicros = micros();
  unsigned long currentMillis = millis();

  if (currentMicros - last_gameUpdate >= interval_gameUpdate) {
    last_gameUpdate = currentMicros;
    animation_index++;
    game_rate_update();
  }

  display.clearDisplayBuffer();
  ScreenMode screen = current_screen_mode();

  if (currentMillis - last_fgUpdate >= interval_fgUpdate) {
    last_fgUpdate = currentMillis;
    fuelGauge_update();
  }

  if (screen == SCREEN_MODE_MAIN) {
    if (zajac) {
      display.drawBitmap(
          SCREEN_WIDTH / 2 - ZAJAC_WIDTH / 2,
          SCREEN_HEIGHT / 2 - ZAJAC_HEIGHT / 2,
          zajac_data[animation_index % ZAJAC_FRAME_COUNT],
          ZAJAC_WIDTH,
          ZAJAC_HEIGHT,
          0);
    } else {
      display.drawBitmap(
          0, 0,
          bob_data[animation_index % BOB_FRAME_COUNT],
          BOB_WIDTH,
          BOB_HEIGHT,
          1, 0);
    }

    if (debug_menu_show_fuel_gauge()) {
      fuelGauge_debug_display();
    }
    battery_display();

    if (debug_menu_show_input_debug()) {
      debug_input_display();
    }
    if (debug_menu_show_refresh_rate()) {
      refresh_rate_debug_display();
    }
  } else if (screen == SCREEN_MODE_DEBUG_MENU) {
    debug_menu_display();
  }

  unsigned long t0 = micros();
  display_refresh_dirty();
  refresh_rate_update(micros() - t0);
}