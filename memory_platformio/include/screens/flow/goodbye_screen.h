#ifndef GOODBYE_SCREEN_H
#define GOODBYE_SCREEN_H

#include <display.h>
#include <game_state.h>
#include <screen.h>

// ---------------------------------------------------------------------------
// GoodbyeScreen
//
// Shows the player's final rank and name after saving.
// Auto-advances to the logo after AUTO_ADVANCE_MS (default 8 s).
// Any button press advances immediately.
//
// draw() is yours to design.
// Available variables:
//   session.player_name
//   session.player_rank
//   session.rounds_count
//   leaderboard_size
//   _elapsed_ms()     — ms since this screen was entered (for countdown display)
// ---------------------------------------------------------------------------
class GoodbyeScreen : public Screen {
  static constexpr uint32_t AUTO_ADVANCE_MS = 8000;

  bool     _done       = false;
  uint32_t _enter_time = 0;

 public:
  ScreenMode id()          const override { return SCREEN_GOODBYE; }
  bool       is_complete() const override { return _done; }

  void on_enter() override {
    _done       = false;
    _enter_time = millis();
  }

  void update() override {
    if (millis() - _enter_time >= AUTO_ADVANCE_MS) _done = true;
  }

  void on_button_a() override { _done = true; }
  void on_button_b() override { _done = true; }

  uint32_t _elapsed_ms() const { return millis() - _enter_time; }

  void draw() override {
    display.fillScreen(WHITE);
    draw_text_block("GOODBYE", 4, 4);
    draw_text_block(String(session.player_name), 4, 28);
    draw_text_block("final rank: #" + String(session.player_rank) +
                    " / " + String(leaderboard_size), 4, 44);
    draw_text_block("rounds played: " + String(session.rounds_count), 4, 60);
    uint32_t secs_left = (AUTO_ADVANCE_MS - min(_elapsed_ms(), AUTO_ADVANCE_MS)) / 1000;
    draw_text_block("returning in " + String(secs_left) + "s...", 4, SCREEN_HEIGHT - 16);
  }
};

#endif
