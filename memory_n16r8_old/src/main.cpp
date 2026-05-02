#include <Arduino.h>

#include <display.h>
#include <fuelGauge.h>
#include <input.h>
#include <screen.h>
#include <screens/bob_screen.h>
#include <screens/debug_menu_screen.h>
#include <screens/grid_screen.h>
#include <screens/zajac_screen.h>
#include <sleep_manager.h>

// ---------------------------------------------------------------------------
// Screen instances — add a new screen here and in set_screen() below.
// ---------------------------------------------------------------------------
BobScreen screen_bob;
ZajacScreen screen_zajac;
DebugMenuScreen screen_debug;
GridScreen screen_grid;

// ---------------------------------------------------------------------------
// Global state — currentScreen is extern-declared in screen.h.
// ---------------------------------------------------------------------------
Screen *currentScreen = &screen_bob;

void set_screen(ScreenMode mode) {
  currentScreen->on_exit();
  switch (mode) {
    case SCREEN_BOB:        currentScreen = &screen_bob;   break;
    case SCREEN_ZAJAC:      currentScreen = &screen_zajac; break;
    case SCREEN_DEBUG_MENU: currentScreen = &screen_debug; break;
    case SCREEN_GRID:       currentScreen = &screen_grid;  break;
  }
  currentScreen->on_enter();
}

// ---------------------------------------------------------------------------
// Debug menu items — all render callbacks must be in scope at this point.
// ---------------------------------------------------------------------------
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
    currentScreen->update();
    game_rate_update();
  }

  display.clearDisplayBuffer();

  if (current_millis - last_fg_update >= interval_fg_update) {
    last_fg_update = current_millis;
    fuel_gauge_update();
  }

  currentScreen->draw();

  if (currentScreen->id() != SCREEN_DEBUG_MENU) {
    battery_display();
    debug_menu.render_overlays();
  }

  unsigned long t0 = micros();
  display_refresh_dirty();
  refresh_rate_update(micros() - t0);
}
