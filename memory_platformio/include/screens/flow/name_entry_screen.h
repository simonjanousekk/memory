#ifndef NAME_ENTRY_SCREEN_H
#define NAME_ENTRY_SCREEN_H

#include <Arduino.h>
#include <display.h>
#include <game_state.h>
#include <screen.h>

class NameEntryScreen : public Screen {
  static constexpr int CHARSET_LEN = 26;
  static constexpr int OK_IDX = CHARSET_LEN;
  static constexpr int SELECT_COUNT = CHARSET_LEN + 1;
  static constexpr int MAX_CHARS = 8;  // session.player_name is char[8]

  String _name = "";
  int _char_idx = 0;  // encoder: selected letter in bottom row
  bool _done = false;
  String _underline = "";
  uint32_t _underline_last_update = 0;
  const uint16_t underline_time_per_update = 500;

  void _finish() {
    strncpy(session.player_name, _name.c_str(), sizeof(session.player_name) - 1);
    session.player_name[sizeof(session.player_name) - 1] = '\0';
    _done = true;
  }

  void _underline_toggle() {
    _underline = _underline.length() == 0 ? "_" : "";
  }

 public:
  ScreenMode id() const override { return SCREEN_NAME_ENTRY; }
  bool is_complete() const override { return _done; }

  void on_enter() override {
    _name = "";
    _char_idx = 0;
    _done = false;
    _underline = "_";
    _underline_last_update = millis();
  }

  void on_encoder_rotate(int delta) override {
    if (delta == 0) return;
    _char_idx = (_char_idx + delta) % SELECT_COUNT;
    if (_char_idx < 0) _char_idx += SELECT_COUNT;
  }

  void on_button_a() override {
    if (_char_idx == OK_IDX) {
      if (_name.length() > 0) _finish();
      return;
    }
    if ((int)_name.length() >= MAX_CHARS) {
      _finish();
      return;
    }
    _done = false;
    _name += char('A' + _char_idx);
    if ((int)_name.length() >= MAX_CHARS) _finish();
  }

  void on_button_b() override {
    if (_name.length() > 0) {
      _name.remove(_name.length() - 1);
      _done = false;
    }
  }

  void on_encoder_press() override {
    if (_name.length() > 0) _finish();
  }

  void update() override {
    if (millis() - _underline_last_update >= underline_time_per_update) {
      _underline_toggle();
      _underline_last_update = millis();
    }
  }

  void draw_letter_key(String letter, int x, int y, bool selected) {
    int y_off = letter == "Q" ? 2 : 0;
    if (selected) {
      display.fillRect(x, y, 24, 24, WHITE);
      display.drawTextCentered(letter, x + 12, y + 12 + y_off, BLACK);
    } else {
      // display.drawRect(x, y, 24, 24, WHITE);
      display.drawTextCentered(letter, x + 12, y + 12 + y_off, WHITE);
    }
  }

  void draw_ok_key(bool selected) {
    int x = 351 + 3;
    int y = SCREEN_HEIGHT - 24 - 3;
    int w = SCREEN_WIDTH - 351 - (3 * 2);
    int h = 24;
    if (selected) {
      display.fillRect(x, y, w, h, WHITE);
      display.drawTextCentered("OK", x + w / 2, y + h / 2, BLACK);
    } else {
      // display.drawRect(x, y, w, h, WHITE);
      display.drawTextCentered("OK", x + w / 2, y + h / 2, WHITE);
    }
  }

  void draw() override {
    display.fillScreen(BLACK);

    display.setFont(&Panell_Regular12pt7b);
    display.setTextColor(WHITE);
    display.setCursor(3, 3 + 16);
    display.print("enter your name.");

    // ---- NAME DISPLAY ----
    display.setFont(&Panell_Extended24pt7b);
    String text = _name + _underline;
    String text_to_measure = text + (_underline == "_" ? "" : "_") + (text == "" ? "A" : "");

    int16_t x1, y1;
    uint16_t w, h;
    // Measure at a safe y to avoid negative coordinate artefacts.
    display.getTextBounds(_name + "A_", 0, 100, &x1, &y1, &w, &h);
    int ascent = 100 - y1;
    // x1 offset (relative to cursor x=0) accounts for left-side bearing.
    display.setCursor(3, SCREEN_HEIGHT_HALF - h / 2 + ascent);
    display.setTextColor(WHITE);
    display.print(text);

    // ---- KEYBOARD + OK key ----
    display.setFont(&Panell_Regular12pt7b);

    const int step = 351 / (CHARSET_LEN / 2);
    const int row_y_1 = SCREEN_HEIGHT - (24 * 2) - (3 * 2);
    const int row_y_2 = SCREEN_HEIGHT - 24 - 3;

    for (int i = 0; i < CHARSET_LEN; i++) {
      const bool selected = (i == _char_idx);
      String letter = String(char('A' + i));

      int x = step * i + 3;
      int y = row_y_1;

      if (!(i < CHARSET_LEN / 2)) {
        x = step * (i - CHARSET_LEN / 2) + 3;
        y = row_y_2;
      }

      draw_letter_key(letter, x, y, selected);
    }

    draw_ok_key(_char_idx == OK_IDX);
  }
};

#endif
