#include <Arduino.h>
#include <display.h>
#include <fuelGauge.h>
#include <game_controller.h>
#include <game_seed.h>
#include <input.h>
#include <screen.h>
// Minigame screens
#include <screens/count_screen.h>
#include <screens/letters_screen.h>
#include <screens/logo_screen.h>
#include <screens/maze/maze_screen.h>
// Game flow screens
#include <screens/flow/go_again_screen.h>
#include <screens/flow/intro_screen.h>
#include <screens/flow/name_entry_screen.h>
#include <screens/flow/opponent_screen.h>
#include <screens/flow/round_result_screen.h>
// Dev / utility screens
#include <screens/bob_screen.h>
#include <screens/debug_menu_screen.h>
#include <screens/grid_screen.h>
#include <screens/test_screen.h>
#include <screens/zajac_screen.h>
#include <sleep_manager.h>
#include <wifi/wifi_manager.h>
// ---------------------------------------------------------------------------
// Screen instances
// ---------------------------------------------------------------------------
// Minigames
MazeScreen screen_maze;
LettersScreen screen_letters;
CountScreen screen_count;
// Game flow
LogoScreen screen_logo;
IntroScreen screen_intro;
OpponentScreen screen_opponent;
RoundResultScreen screen_result;
GoAgainScreen screen_go_again;
NameEntryScreen screen_name_entry;
// Dev / utility
BobScreen screen_bob;
ZajacScreen screen_zajac;
DebugMenuScreen screen_debug;
GridScreen screen_grid;
TestScreen screen_test;

// ---------------------------------------------------------------------------
// Global state
// ---------------------------------------------------------------------------
Screen* currentScreen = &screen_logo;

void set_screen(ScreenMode mode) {
  currentScreen->on_exit();
  switch (mode) {
    // Minigames
    case SCREEN_MAZE:
      currentScreen = &screen_maze;
      break;
    case SCREEN_LETTERS:
      currentScreen = &screen_letters;
      break;
    case SCREEN_COUNT:
      currentScreen = &screen_count;
      break;
    // Game flow
    case SCREEN_LOGO:
      currentScreen = &screen_logo;
      break;
    case SCREEN_INTRO:
      currentScreen = &screen_intro;
      break;
    case SCREEN_OPPONENT:
      currentScreen = &screen_opponent;
      break;
    case SCREEN_RESULT:
      currentScreen = &screen_result;
      break;
    case SCREEN_GO_AGAIN:
      currentScreen = &screen_go_again;
      break;
    case SCREEN_NAME_ENTRY:
      currentScreen = &screen_name_entry;
      break;
    // Dev / utility
    case SCREEN_BOB:
      currentScreen = &screen_bob;
      break;
    case SCREEN_ZAJAC:
      currentScreen = &screen_zajac;
      break;
    case SCREEN_DEBUG_MENU:
      currentScreen = &screen_debug;
      break;
    case SCREEN_GRID:
      currentScreen = &screen_grid;
      break;
    case SCREEN_TEST:
      currentScreen = &screen_test;
      break;
    default:
      currentScreen = &screen_logo;
      break;
  }
  currentScreen->ensure_init();
  currentScreen->on_enter();
}

// ---------------------------------------------------------------------------
// Debug menu items
// ---------------------------------------------------------------------------
DebugMenuItem debug_items[] = {
    DebugMenuItem("Fuel gauge", fuel_gauge_debug_display),
    DebugMenuItem("Refresh rate", _dbg_render_refresh_rate),
    DebugMenuItem("Input debug", debug_input_display),
    DebugMenuItem("WiFi", nullptr, wifi_manager_start, wifi_manager_reset),
    DebugMenuItem("WiFi debug", wifi_debug_display),
    DebugMenuItem("Seed", &g_game_seed),
    DebugMenuItem("-> Maze", []() { set_screen(SCREEN_MAZE); }),
    DebugMenuItem("-> Letters", []() { set_screen(SCREEN_LETTERS); }),
    DebugMenuItem("-> Count", []() { set_screen(SCREEN_COUNT); }),
    DebugMenuItem("-> Bob", []() { set_screen(SCREEN_BOB); }),
    DebugMenuItem("-> Zajac", []() { set_screen(SCREEN_ZAJAC); }),
    DebugMenuItem("-> Grid", []() { set_screen(SCREEN_GRID); }),
    DebugMenuItem("-> Logo", []() { logo_set_mode(LOGO_BOOT); set_screen(SCREEN_LOGO); }),
    DebugMenuItem("-> Test", []() { set_screen(SCREEN_TEST); }),
    DebugMenuItem("-> Name Entry", []() { game_controller.advance(PHASE_NAME_ENTRY); }),
};

void setup() {
  Serial.begin(115200);
  delay(100);

  debug_menu.init(debug_items, sizeof(debug_items) / sizeof(debug_items[0]));

  display_init();
  currentScreen->ensure_init();
  currentScreen->on_enter();
  input_init();
  sleep_init();
  fuel_gauge_init();
  fuel_gauge_update();

  wifi_manager_start();     // begin connecting; logo screen polls completion
  game_controller.start();  // drives all phase transitions
}

unsigned long last_game_update = 0;
const unsigned long interval_game_update = 1000000 / 30;
unsigned long last_display_refresh = 0;
const unsigned long interval_display_refresh = 1000000 / 30;
unsigned long last_fg_update = -9000;
const unsigned long interval_fg_update = 10000;

void loop() {
  unsigned long current_micros = micros();
  unsigned long current_millis = millis();

  input_update();
  wifi_manager_update();

  if (current_micros - last_game_update >= interval_game_update) {
    last_game_update = current_micros;
    currentScreen->update();
    game_controller.update();
    game_rate_update();
  }

  if (current_micros - last_display_refresh < interval_display_refresh)
    return;

  last_display_refresh = current_micros;
  display.fillScreen(WHITE);

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

  input_update();
}
