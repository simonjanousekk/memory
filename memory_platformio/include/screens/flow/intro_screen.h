#ifndef INTRO_SCREEN_H
#define INTRO_SCREEN_H

#include <display.h>
#include <game_state.h>
#include <screen.h>

#include "screens/test_screen.h"

// ---------------------------------------------------------------------------
// IntroScreen
//
// Static explainer shown once per session after the leaderboard is loaded.
// Press A or B to start.  The first round is always a solo tutorial run
// (random seeds, no opponent) — subsequent rounds challenge real opponents.
// ---------------------------------------------------------------------------
class IntroScreen : public Screen {
  bool _done = false;

 public:
  ScreenMode id() const override { return SCREEN_INTRO; }
  bool is_complete() const override { return _done; }

  void on_enter() override { _done = false; }
  void on_button_a() override { _done = true; }
  void on_button_b() override { _done = true; }

  void draw() override {
    display.fillScreen(BLACK);
    display.setFont(&Panell_Extended24pt7b);
    display.drawTextCentered("ready?", SCREEN_WIDTH_HALF, SCREEN_HEIGHT_HALF - 16, WHITE);
    display.setFont(&Panell_Regular12pt7b);
    display.drawTextCentered("play 3 minigames", SCREEN_WIDTH_HALF, SCREEN_HEIGHT_HALF + 24, WHITE);
    display.drawTextCentered("climb the leaderboard", SCREEN_WIDTH_HALF, SCREEN_HEIGHT_HALF + 48, WHITE);

    // display.fillScreen(WHITE);
    // draw_text_block("MEMORY", 4, 4);
    // if (session.offline)
    //   draw_text_block("OFFLINE MODE", 4, 20);
    // draw_text_block("players: " + String(leaderboard_size), 4, 36);
    // draw_text_block("your start rank: #" + String(session.player_rank), 4, 52);
    // draw_text_block("first: solo tutorial run (3 games)", 4, 76);
    // draw_text_block("then: challenge players, climb board", 4, 92);
    // draw_text_block("win = rank up   lose = rank down", 4, 108);
    // draw_text_block("A to start", 4, SCREEN_HEIGHT - 16);
  }
};

#endif
