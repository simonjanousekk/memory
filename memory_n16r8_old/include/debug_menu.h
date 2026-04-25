#ifndef DEBUG_MENU_H
#define DEBUG_MENU_H

#include <Arduino.h>
#include <display.h>

enum DebugMenuItem {
  DEBUG_MENU_ITEM_FUEL_GAUGE = 0,
  DEBUG_MENU_ITEM_REFRESH_RATE = 1,
  DEBUG_MENU_ITEM_INPUT_DEBUG = 2,
  DEBUG_MENU_ITEM_COUNT = 3
};

enum ScreenMode {
  SCREEN_MODE_MAIN = 0,
  SCREEN_MODE_DEBUG_MENU = 1
};

struct DebugMenuState {
  ScreenMode screen_mode = SCREEN_MODE_MAIN;
  int selected_index = 0;
  bool show_fuel_gauge = false;
  bool show_input_debug = false;
  bool show_refresh_rate = false;
};

DebugMenuState debug_menu_state;

bool debug_menu_is_open() {
  return debug_menu_state.screen_mode == SCREEN_MODE_DEBUG_MENU;
}

ScreenMode current_screen_mode() {
  return debug_menu_state.screen_mode;
}

bool debug_menu_show_fuel_gauge() {
  return debug_menu_state.show_fuel_gauge;
}

bool debug_menu_show_input_debug() {
  return debug_menu_state.show_input_debug;
}

bool debug_menu_show_refresh_rate() {
  return debug_menu_state.show_refresh_rate;
}

void debug_menu_enter_or_exit() {
  if (debug_menu_state.screen_mode == SCREEN_MODE_DEBUG_MENU) {
    debug_menu_state.screen_mode = SCREEN_MODE_MAIN;
    return;
  }

  debug_menu_state.screen_mode = SCREEN_MODE_DEBUG_MENU;
}

void debug_menu_exit() {
  if (debug_menu_state.screen_mode != SCREEN_MODE_DEBUG_MENU) {
    return;
  }
  debug_menu_state.screen_mode = SCREEN_MODE_MAIN;
}

void debug_menu_encoder_step(int delta) {
  if (debug_menu_state.screen_mode != SCREEN_MODE_DEBUG_MENU || delta == 0) {
    return;
  }

  debug_menu_state.selected_index += delta;
  while (debug_menu_state.selected_index < 0) {
    debug_menu_state.selected_index += DEBUG_MENU_ITEM_COUNT;
  }
  debug_menu_state.selected_index %= DEBUG_MENU_ITEM_COUNT;
}

void debug_menu_confirm_selected() {
  if (debug_menu_state.screen_mode != SCREEN_MODE_DEBUG_MENU) {
    return;
  }

  if (debug_menu_state.selected_index == DEBUG_MENU_ITEM_FUEL_GAUGE) {
    debug_menu_state.show_fuel_gauge = !debug_menu_state.show_fuel_gauge;
    return;
  }

  if (debug_menu_state.selected_index == DEBUG_MENU_ITEM_INPUT_DEBUG) {
    debug_menu_state.show_input_debug = !debug_menu_state.show_input_debug;
    return;
  }

  if (debug_menu_state.selected_index == DEBUG_MENU_ITEM_REFRESH_RATE) {
    debug_menu_state.show_refresh_rate = !debug_menu_state.show_refresh_rate;
  }
}

void debug_menu_draw_row(int y, bool selected, const char *label, bool enabled) {
  if (selected) {
    // Measure actual text bounds so the highlight fits any font.
    char sample[64];
    snprintf(sample, sizeof(sample), "> %s: ON", label);
    int16_t x1, y1;
    uint16_t w, h;
    display.setTextSize(1);
    display.getTextBounds(sample, 40, y, &x1, &y1, &w, &h);
    display.fillRect(30, y1 - 2, SCREEN_WIDTH - 60, h + 4, BLACK);
    display.setTextColor(WHITE);
  } else {
    display.setTextColor(BLACK);
  }

  display.setCursor(40, y);
  display.print(selected ? "> " : "  ");
  display.print(label);
  display.print(": ");
  display.print(enabled ? "ON" : "OFF");
}

void debug_menu_display() {
  if (debug_menu_state.screen_mode != SCREEN_MODE_DEBUG_MENU) {
    return;
  }

  display.drawRect(20, 20, SCREEN_WIDTH - 40, SCREEN_HEIGHT - 40, BLACK);
  display.setTextSize(1);
  display.setTextColor(BLACK);

  display.setCursor(40, 40);
  display.print("DEBUG MENU");

  debug_menu_draw_row(80, debug_menu_state.selected_index == DEBUG_MENU_ITEM_FUEL_GAUGE,
                      "Fuel gauge", debug_menu_state.show_fuel_gauge);
  debug_menu_draw_row(110, debug_menu_state.selected_index == DEBUG_MENU_ITEM_REFRESH_RATE,
                      "Refresh rate", debug_menu_state.show_refresh_rate);
  debug_menu_draw_row(140, debug_menu_state.selected_index == DEBUG_MENU_ITEM_INPUT_DEBUG,
                      "Input debug", debug_menu_state.show_input_debug);
}

#endif
