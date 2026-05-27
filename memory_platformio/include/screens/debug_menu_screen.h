#ifndef DEBUG_MENU_SCREEN_H
#define DEBUG_MENU_SCREEN_H

#include <Arduino.h>
#include <display.h>
#include <screen.h>

// Render callback for the "Refresh rate" debug item.
// Lives here because it reads _rr_avg_ms / _gr_tps from display.h.
//   ms   : avg time inside display_refresh_dirty() (mostly no-op memcmp
//          between game ticks since the canvas is byte-identical)
//   FPS  : theoretical max if every refresh were the no-op cost (1000/ms)
//   UPS  : actual game tick rate (currentScreen->update() calls per second)
static void _dbg_render_refresh_rate(char *buf, int len) {
  snprintf(buf, len, "%.1fms %.0fFPS %.0fUPS", _rr_avg_ms,
           1000.0f / max(_rr_avg_ms, 0.001f), _gr_tps);
}

// ---------------------------------------------------------------------------
// DebugMenuItem
// ---------------------------------------------------------------------------
class DebugMenuItem {
public:
  const char *label;
  char label_fixed_width[20];
  bool enabled;

  void (*render)(char *buf, int len);
  void (*on_enable)();
  void (*on_disable)();
  void (*action)(); // if set: one-shot action item, no toggle state
  uint32_t *value_ptr; // if set: editable numeric value item

  DebugMenuItem()
      : label(""), enabled(false), render(nullptr), on_enable(nullptr),
        on_disable(nullptr), action(nullptr), value_ptr(nullptr) {
    label_fixed_width[0] = '\0';
  }
  // Toggle / overlay item
  DebugMenuItem(const char *label, void (*render)(char *buf, int len) = nullptr,
                void (*on_enable)() = nullptr, void (*on_disable)() = nullptr)
      : label(label), enabled(false), render(render), on_enable(on_enable),
        on_disable(on_disable), action(nullptr), value_ptr(nullptr) {
    label_fixed_width[0] = '\0';
  }
  // Action item — second param type (void(*)()) is distinct from render
  DebugMenuItem(const char *label, void (*action)())
      : label(label), enabled(false), render(nullptr), on_enable(nullptr),
        on_disable(nullptr), action(action), value_ptr(nullptr) {
    label_fixed_width[0] = '\0';
  }
  // Editable value item — Button A enters editing, encoder changes value, B exits
  DebugMenuItem(const char *label, uint32_t *value)
      : label(label), enabled(false), render(nullptr), on_enable(nullptr),
        on_disable(nullptr), action(nullptr), value_ptr(value) {
    label_fixed_width[0] = '\0';
  }

  void toggle() {
    if (action) {
      action();
      return;
    }
    if (value_ptr)
      return; // editing is handled by DebugMenu directly
    enabled = !enabled;
    if (enabled && on_enable)
      on_enable();
    if (!enabled && on_disable)
      on_disable();
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
  bool _editing = false; // true while rotating encoder changes a value item

  void init(DebugMenuItem *items_, int count) {
    items = items_;
    item_count = count;

    int col1_count = (item_count + 1) / 2;
    int max_len_col1 = 0;
    int max_len_col2 = 0;
    for (int i = 0; i < item_count; i++) {
      int l = strlen(items[i].label);
      if (i < col1_count) {
        if (l > max_len_col1)
          max_len_col1 = l;
      } else {
        if (l > max_len_col2)
          max_len_col2 = l;
      }
    }

    for (int i = 0; i < item_count; i++) {
      int max_len = (i < col1_count) ? max_len_col1 : max_len_col2;
      int l = strlen(items[i].label);
      int pad = max_len - l;
      int capacity = (int)sizeof(items[i].label_fixed_width) - 1;
      int copy_len = l < capacity ? l : capacity;
      memcpy(items[i].label_fixed_width, items[i].label, copy_len);
      int space_end = copy_len + pad < capacity ? copy_len + pad : capacity;
      for (int j = copy_len; j < space_end; j++)
        items[i].label_fixed_width[j] = ' ';
      items[i].label_fixed_width[space_end] = '\0';
    }
  }

  void encoder_step(int delta) {
    if (delta == 0)
      return;
    if (_editing && items && items[selected_index].value_ptr) {
      *items[selected_index].value_ptr += (uint32_t)delta;
      return;
    }
    if (item_count == 0)
      return;
    selected_index =
        ((selected_index + delta) % item_count + item_count) % item_count;
  }

  void confirm_selected() {
    if (!items)
      return;
    if (items[selected_index].value_ptr) {
      _editing = !_editing;
      return;
    }
    _editing = false;
    items[selected_index].toggle();
  }

  void cancel_edit() { _editing = false; }

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
    display.setTextSize(1);
    display.setTextColor(BLACK);
    const int border = 5;
    const int row_h = font_height + 6;
    const int col1_count = (item_count + 1) / 2;
    const int col2_x = SCREEN_WIDTH_HALF + border;
    char row_buf[32];

    for (int i = 0; i < item_count; i++) {
      if (items[i].value_ptr) {
        bool editing_this = (_editing && i == selected_index);
        snprintf(row_buf, sizeof(row_buf), "%s %s%lu%s",
                 items[i].label_fixed_width,
                 editing_this ? "[" : " ",
                 (unsigned long)*items[i].value_ptr,
                 editing_this ? "]" : " ");
      } else if (items[i].action) {
        snprintf(row_buf, sizeof(row_buf), "%s >", items[i].label_fixed_width);
      } else {
        snprintf(row_buf, sizeof(row_buf), "%s - %s", items[i].label_fixed_width,
                 items[i].enabled ? " ON" : "OFF");
      }

      const int col = (i < col1_count) ? 0 : 1;
      const int row = (col == 0) ? i : (i - col1_count);
      const int x = (col == 0) ? border : col2_x;
      const int y = border + row * row_h;
      draw_text_block(row_buf, x, y, i == selected_index ? WHITE : BLACK);
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
  void on_button_b() override {
    if (debug_menu._editing)
      debug_menu.cancel_edit();
    else
      toggle_debug_menu();
  }
  void on_encoder_press() override { debug_menu.confirm_selected(); }
  void on_encoder_rotate(int delta) override { debug_menu.encoder_step(delta); }
};

#endif
