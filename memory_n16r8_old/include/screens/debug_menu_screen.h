#ifndef DEBUG_MENU_SCREEN_H
#define DEBUG_MENU_SCREEN_H

#include <Arduino.h>
#include <display.h>
#include <screen.h>

// Render callback for the "Refresh rate" debug item.
// Lives here because it reads _rr_avg_ms / _gr_tps from display.h.
static void _dbg_render_refresh_rate() {
  char buf[40];
  snprintf(buf, sizeof(buf), "%.1fms %.0fUPS %.0fFPS", _rr_avg_ms,
           1000.0f / max(_rr_avg_ms, 0.001f), _gr_tps);
  draw_text_block(buf, 1, 18, WHITE);
}

// ---------------------------------------------------------------------------
// DebugMenuItem
// ---------------------------------------------------------------------------
class DebugMenuItem {
public:
  const char *label;
  bool enabled;
  void (*render)();

  DebugMenuItem() : label(""), enabled(false), render(nullptr) {}
  DebugMenuItem(const char *label, void (*render)() = nullptr)
      : label(label), enabled(false), render(render) {}

  void toggle() { enabled = !enabled; }

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

  void render_overlay() const {
    if (enabled && render)
      render();
  }
};

// ---------------------------------------------------------------------------
// DebugMenu — item list, navigation, drawing.
// ---------------------------------------------------------------------------
class DebugMenu {
public:
  int selected_index = 0;
  DebugMenuItem *items = nullptr;
  int item_count = 0;

  void init(DebugMenuItem *items_, int count) {
    items = items_;
    item_count = count;
  }

  void encoder_step(int delta) {
    if (delta == 0 || item_count == 0)
      return;
    selected_index =
        ((selected_index + delta) % item_count + item_count) % item_count;
  }

  void confirm_selected() {
    if (!items)
      return;
    items[selected_index].toggle();
  }

  void render_overlays() const {
    if (!items)
      return;
    for (int i = 0; i < item_count; i++)
      items[i].render_overlay();
  }

  void draw() const {
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

// ---------------------------------------------------------------------------
// DebugMenuScreen — the full-screen debug menu, extends Screen.
// ---------------------------------------------------------------------------
class DebugMenuScreen : public Screen {
public:
  ScreenMode id() const override { return SCREEN_DEBUG_MENU; }
  void draw() override { debug_menu.draw(); }
};

#endif
