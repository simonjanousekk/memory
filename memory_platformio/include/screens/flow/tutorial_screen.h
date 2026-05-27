#ifndef TUTORIAL_SCREEN_H
#define TUTORIAL_SCREEN_H

#include <display.h>
#include <game_state.h>
#include <screen.h>

// ---------------------------------------------------------------------------
// TutorialScreen
//
// One parameterised class used for all three minigame tutorials.
// Constructed with a fixed ScreenMode ID, game name, and controls text.
// The opponent time for the minigame is read from current_round at draw time.
//
// Three instances are declared in main.cpp:
//   screen_tutorial_maze, screen_tutorial_letters, screen_tutorial_count
//
// Press A or B to continue to the minigame.
//
// draw() is yours to design.
// Available variables:
//   _minigame_name   — "MAZE" / "WORDS" / "COUNT"
//   _controls        — controls description string
//   opponent_time()  — opponent's time for this minigame in ms
//   session.player_rank
// ---------------------------------------------------------------------------
class TutorialScreen : public Screen {
  ScreenMode  _id;
  const char* _minigame_name;
  const char* _controls;
  bool        _done = false;

 public:
  TutorialScreen(ScreenMode id, const char* name, const char* controls)
      : _id(id), _minigame_name(name), _controls(controls) {}

  ScreenMode id()          const override { return _id; }
  bool       is_complete() const override { return _done; }

  void on_enter()    override { _done = false; }
  void on_button_a() override { _done = true; }
  void on_button_b() override { _done = true; }

  // Returns the opponent's reference time for whichever minigame this is.
  uint32_t opponent_time() const {
    if (_id == SCREEN_TUTORIAL_MAZE)    return current_round.maze_opponent_time;
    if (_id == SCREEN_TUTORIAL_LETTERS) return current_round.words_opponent_time;
    return current_round.count_opponent_time;
  }

  void draw() override {
    display.fillScreen(WHITE);
    draw_text_block(_minigame_name, 4, 4);
    draw_text_block(_controls, 4, 24);
    draw_text_block("target: " + format_ms(opponent_time()), 4, 60);
    draw_text_block("rank: #" + String(session.player_rank), 4, 76);
    draw_text_block("A to play", 4, SCREEN_HEIGHT - 16);
  }
};

#endif
