#ifndef DEBUG_MENU_H
#define DEBUG_MENU_H

#include <Arduino.h>
#include <display.h>

enum ScreenMode {
  SCREEN_MODE_MAIN = 0,
  SCREEN_MODE_DEBUG_MENU = 1
};

// Render callback for the "Refresh rate" item — lives here because its data
// (_rr_avgMs, _gr_tps) and helpers (draw_text_block) come from display.h.
static void _dbg_render_refresh_rate() {
  char buf[40];
  snprintf(buf, sizeof(buf), "%.1fms %.0fUPS %.0fFPS",
           _rr_avg_ms, 1000.0f / max(_rr_avg_ms, 0.001f), _gr_tps);
  draw_text_block(buf, 1, 18, WHITE);
}

// ---------------------------------------------------------------------------
// DebugMenuItem
// Each item owns its label, enabled state, and its main-screen overlay render.
// ---------------------------------------------------------------------------
class DebugMenuItem {
public:
  const char *label;
  bool        enabled;
  void (*render)();

  DebugMenuItem() : label(""), enabled(false), render(nullptr) {}
  DebugMenuItem(const char *label, void (*render)() = nullptr)
      : label(label), enabled(false), render(render) {}

  void toggle() { enabled = !enabled; }

  // Draws one row inside the debug menu overlay.
  void draw_row(int y, bool selected) const {
    if (selected) {
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

  // Called every frame from the main screen; no-ops when disabled.
  void render_overlay() const {
    if (enabled && render) render();
  }
};

// ---------------------------------------------------------------------------
// DebugMenu
// Holds the item list and all navigation / drawing logic.
// ---------------------------------------------------------------------------
class DebugMenu {
public:
  ScreenMode    screen_mode    = SCREEN_MODE_MAIN;
  int           selected_index = 0;
  DebugMenuItem *items         = nullptr;
  int           item_count     = 0;

  // Call once in setup(), after all render functions are in scope.
  void init(DebugMenuItem *items_, int count) {
    items      = items_;
    item_count = count;
  }

  bool is_open() const { return screen_mode == SCREEN_MODE_DEBUG_MENU; }

  void enter_or_exit() {
    screen_mode = is_open() ? SCREEN_MODE_MAIN : SCREEN_MODE_DEBUG_MENU;
  }

  void exit() { screen_mode = SCREEN_MODE_MAIN; }

  void encoder_step(int delta) {
    if (!is_open() || delta == 0 || item_count == 0) return;
    selected_index =
        ((selected_index + delta) % item_count + item_count) % item_count;
  }

  void confirm_selected() {
    if (!is_open() || !items) return;
    items[selected_index].toggle();
  }

  // Call from the main-screen branch to run every enabled item's overlay.
  void render_overlays() const {
    if (!items) return;
    for (int i = 0; i < item_count; i++) items[i].render_overlay();
  }

  // Call from the debug-menu branch to draw the full menu.
  void draw() const {
    if (!is_open()) return;

    display.drawRect(20, 20, SCREEN_WIDTH - 40, SCREEN_HEIGHT - 40, BLACK);
    display.setTextSize(1);
    display.setTextColor(BLACK);
    display.setCursor(40, 40);
    display.print("DEBUG MENU");

    int row_y = 80;
    for (int i = 0; i < item_count; i++) {
      items[i].draw_row(row_y, i == selected_index);
      row_y += 30;
    }
  }
};

DebugMenu debug_menu;

#endif
