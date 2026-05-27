#ifndef NAME_ENTRY_SCREEN_H
#define NAME_ENTRY_SCREEN_H

#include <display.h>
#include <game_state.h>
#include <screen.h>

class NameEntryScreen : public Screen {
  static constexpr int CHARSET_LEN = 26;
  static constexpr int MAX_CHARS = 7;  // session.player_name is char[8]

  String _name = "";
  int _char_idx = 0;  // encoder: selected letter in bottom row
  bool _done = false;
  String _underline = "";
  uint32_t _underline_last_update = 0;
  const uint16_t underline_time_per_update = 1000;

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
    _char_idx = (_char_idx + delta) % CHARSET_LEN;
    if (_char_idx < 0) _char_idx += CHARSET_LEN;
  }

  void on_button_a() override {
    if ((int)_name.length() >= MAX_CHARS) return;
    _name += char('A' + _char_idx);
    if ((int)_name.length() >= MAX_CHARS) _finish();
  }

  void on_button_b() override {
    if (_name.length() > 0)
      _name.remove(_name.length() - 1);
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
    if (selected) {
      display.fillRect(x, y, 24, 24, WHITE);
      display.drawTextCentered(letter, x + 12, y + 12, BLACK);
    } else {
      display.drawRect(x, y, 24, 24, WHITE);
      display.drawTextCentered(letter, x + 12, y + 12, WHITE);
    }
  }

  void
  draw() override {
    display.fillScreen(BLACK);

    display.setFont(&Panell_Extended24pt7b);
    display.drawTextCentered(_name + _underline, SCREEN_WIDTH_HALF,
        SCREEN_HEIGHT_HALF - 16, WHITE);

    display.setFont(&Panell_Regular12pt7b);

    const int step = SCREEN_WIDTH / (CHARSET_LEN / 2);
    const int row_y_1 = SCREEN_HEIGHT - 64;
    const int row_y_2 = SCREEN_HEIGHT - 32;

    for (int i = 0; i < CHARSET_LEN; i++) {
      const bool selected = (i == _char_idx);
      String letter = String(char('A' + i));

      if (i < CHARSET_LEN / 2) {
        draw_letter_key(letter, step * i, row_y_1, selected);
      } else {
        draw_letter_key(letter, step * (i - CHARSET_LEN / 2), row_y_2, selected);
      }
    }
  }
};

#endif
