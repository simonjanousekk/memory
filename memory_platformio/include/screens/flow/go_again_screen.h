#ifndef GO_AGAIN_SCREEN_H
#define GO_AGAIN_SCREEN_H

#include <display.h>
#include <game_state.h>
#include <screen.h>

// ---------------------------------------------------------------------------
// GoAgainScreen
//
// After seeing the round result the player chooses:
//   Button A → GO AGAIN  (complete_result() == 1 → controller goes to OPPONENT)
//   Button B → KEEP RANK (complete_result() == 0 → controller goes to NAME_ENTRY)
//
// draw() is yours to design.
// Available variables:
//   session.player_rank    — current rank (result of this round)
//   session.rounds_count   — total rounds played this session
//   leaderboard_size       — total players
//   _choice                — 0 = keep, 1 = go again (set on button press)
//   at_top()               — true when player is at rank 1 (show crown warning)
// ---------------------------------------------------------------------------
class GoAgainScreen : public Screen {
  bool _done = false;
  int _choice = 0;  // 0 = keep, 1 = go again

 public:
  ScreenMode id() const override { return SCREEN_GO_AGAIN; }
  bool is_complete() const override { return _done; }
  int complete_result() const override { return _choice; }

  void on_enter() override {
    _done = false;
    _choice = 0;
  }

  void on_button_a() override { _done = true; }

  void on_encoder_rotate(int delta) override {
    if (delta == 0) return;
    _choice = (_choice + delta) % 2;
    if (_choice < 0) _choice += 2;
  }

  bool at_top() const { return session.player_rank == 1; }

  void draw() override {
    // display.fillScreen(WHITE);
    // draw_text_block("YOUR RANK: #" + String(session.player_rank), 4, 4);
    // if (at_top())
    //   draw_text_block("*** RANK 1 — defend the crown! ***", 4, 24);
    // draw_text_block("rounds this session: " + String(session.rounds_count), 4, 44);
    // draw_text_block("board size: " + String(leaderboard_size), 4, 60);
    // draw_text_block("A = GO AGAIN  (risk your rank)", 4, 88);
    // draw_text_block("B = KEEP RANK (save & quit)", 4, 104);
    display.fillScreen(BLACK);
    display.setFont(&Panell_Extended24pt7b);
    display.drawTextCentered("go again?", SCREEN_WIDTH_HALF, SCREEN_HEIGHT_HALF - 16, WHITE);
    display.setFont(&Panell_Regular12pt7b);
    display.drawTextCentered(
        "YES",
        SCREEN_WIDTH_HALF - 32, SCREEN_HEIGHT_HALF + 48,
        _choice == 1 ? BLACK : WHITE,
        _choice == 1,
        // _choice != 1,
        false,
        6);
    display.drawTextCentered(
        "NO",
        SCREEN_WIDTH_HALF + 32, SCREEN_HEIGHT_HALF + 48,
        _choice == 0 ? BLACK : WHITE,
        _choice == 0,
        // _choice != 0,
        false,
        6);
  }
};

#endif
