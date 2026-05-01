#include <Arduino.h>
#include <debug_menu.h>
#include <display.h>
#include <fuelGauge.h>
#include <input.h>
#include <sleep_manager.h>
#include <sprites/bob.h>
#include <sprites/zajac.h>

// All render callbacks are now in scope — define the item list here.
DebugMenuItem debug_items[] = {
    DebugMenuItem("Fuel gauge", fuel_gauge_debug_display),
    DebugMenuItem("Refresh rate", _dbg_render_refresh_rate),
    DebugMenuItem("Input debug", debug_input_display),
};

void setup() {
  Serial.begin(115200);
  delay(100);

  debug_menu.init(debug_items, sizeof(debug_items) / sizeof(debug_items[0]));

  display_init();
  input_init();
  sleep_init();
  fuel_gauge_init();
  fuel_gauge_update();
}

uint16_t animation_index = 0;
unsigned long last_game_update = 0;
const unsigned long interval_game_update = 1000000 / 12;

unsigned long last_fg_update = -9000;
const unsigned long interval_fg_update = 10000;

void loop() {
  unsigned long current_micros = micros();
  unsigned long current_millis = millis();

  input_update();

  if (current_micros - last_game_update >= interval_game_update) {
    last_game_update = current_micros;
    animation_index++;
    game_rate_update();
  }

  display.clearDisplayBuffer();

  if (current_millis - last_fg_update >= interval_fg_update) {
    last_fg_update = current_millis;
    fuel_gauge_update();
  }

  if (debug_menu.screen_mode == SCREEN_MODE_MAIN) {
    if (zajac) {
      display.drawBitmap(SCREEN_WIDTH / 2 - ZAJAC_WIDTH / 2,
                         SCREEN_HEIGHT / 2 - ZAJAC_HEIGHT / 2,
                         zajac_data[animation_index % ZAJAC_FRAME_COUNT],
                         ZAJAC_WIDTH, ZAJAC_HEIGHT, 0);
    } else {
      display.drawBitmap(0, 0, bob_data[animation_index % BOB_FRAME_COUNT],
                         BOB_WIDTH, BOB_HEIGHT, 1, 0);
    }

    battery_display();
    debug_menu.render_overlays();
  } else if (debug_menu.screen_mode == SCREEN_MODE_DEBUG_MENU) {
    debug_menu.draw();
  }

  unsigned long t0 = micros();
  display_refresh_dirty();
  refresh_rate_update(micros() - t0);
}
