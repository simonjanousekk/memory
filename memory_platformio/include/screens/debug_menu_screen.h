#ifndef DEBUG_MENU_SCREEN_H
#define DEBUG_MENU_SCREEN_H

#include <Arduino.h>
#include <display.h>
#include <screen.h>

// Render callback for the "Refresh rate" debug item.
// Lives here because it reads _rr_avg_ms / _gr_tps from display.h.
static void _dbg_render_refresh_rate(char *buf, int len) {
  snprintf(buf, len, "%.1fms %.0fUPS %.0fFPS", _rr_avg_ms,
           1000.0f / max(_rr_avg_ms, 0.001f), _gr_tps);
}

// ---------------------------------------------------------------------------
// DebugMenuItem
// ---------------------------------------------------------------------------
class DebugMenuItem {
public:
  const char *label;
  bool enabled;
  void (*render)(char *buf, int len);
  void (*on_enable)();
  void (*on_disable)();

  DebugMenuItem()
      : label(""), enabled(false), render(nullptr), on_enable(nullptr),
        on_disable(nullptr) {}
  DebugMenuItem(const char *label, void (*render)(char *buf, int len) = nullptr,
                void (*on_enable)() = nullptr, void (*on_disable)() = nullptr)
      : label(label), enabled(false), render(render), on_enable(on_enable),
        on_disable(on_disable) {}

  void toggle() {
    enabled = !enabled;
    if (enabled && on_enable)
      on_enable();
    if (!enabled && on_disable)
      on_disable();
  }

  void draw_row(int x, int y, bool selected) const {
    if (selected) {
      // char sample[64];
      // snprintf(sample, sizeof(sample), "> %s: ON", label);
      // int16_t x1, y1;
      // uint16_t w, h;
      // display.setTextSize(1);
      // display.getTextBounds(sample, 40, y, &x1, &y1, &w, &h);
      // display.fillRect(30, y1 - 2, SCREEN_WIDTH - 60, h + 4, BLACK);
      display.fillRect(x, y - font_height, SCREEN_WIDTH_HALF, font_height + 6,
                       BLACK);
      display.setTextColor(WHITE);
    } else {
      display.setTextColor(BLACK);
    }
    display.setCursor(x, y);
    // display.print(selected ? "> " : "  ");

    display.print(label);
    display.print(": ");
    display.setCursor(SCREEN_WIDTH_HALF - 35, y);
    display.print(enabled ? "ON" : "OFF");
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
    char buf[64];
    int y = 1;
    for (int i = 0; i < item_count; i++) {
      if (!items[i].enabled || !items[i].render)
        continue;
      buf[0] = '\0';
      items[i].render(buf, sizeof(buf));
      if (buf[0] != '\0') {
        draw_text_block(buf, 1, y, WHITE);
        y += font_height + 6; // box height (font + 2*border) + 2px gap
      }
    }
  }

  void draw() const {
    // display.drawRect(20, 20, SCREEN_WIDTH - 40, SCREEN_HEIGHT - 40, BLACK);
    display.setTextSize(1);
    display.setTextColor(BLACK);
    // display.setCursor(40, 40);
    // display.print("DEBUG MENU");
    int border = 5;
    int row_y = font_height + border;
    for (int i = 0; i < item_count; i++) {
      items[i].draw_row(border, row_y, i == selected_index);
      row_y += font_height + 6;
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
  void on_button_a() override { debug_menu.confirm_selected(); }
  void on_button_b() override { toggle_debug_menu(); }
  void on_encoder_press() override { debug_menu.confirm_selected(); }
  void on_encoder_rotate(int delta) override { debug_menu.encoder_step(delta); }
};

#endif
