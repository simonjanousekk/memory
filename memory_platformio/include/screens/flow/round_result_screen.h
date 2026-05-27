#ifndef ROUND_RESULT_SCREEN_H
#define ROUND_RESULT_SCREEN_H

#include <display.h>
#include <game_state.h>
#include <screen.h>

// ---------------------------------------------------------------------------
// RoundResultScreen
//
// Shows the per-minigame comparison and the overall rank movement after a
// round.  session.commit_round() has already run when this screen is entered,
// so session.player_rank is the NEW rank.
//
// Press A to continue to the Go-Again screen.
//
// draw() is yours to design.
// Available variables:
//   current_round.maze_player_time    — your maze time (ms)
//   current_round.maze_opponent_time  — opponent's maze time (ms)
//   current_round.words_player_time   — your letters time (ms)
//   current_round.words_opponent_time — opponent's letters time (ms)
//   current_round.count_player_time   — your count time (ms)
//   current_round.count_opponent_time — opponent's count time (ms)
//   current_round.player_wins         — 0–3
//   current_round.rank_delta          — –3 / –1 / +1 / +3
//   session.player_rank               — NEW rank (after this round)
//   prev_rank()                       — OLD rank (before this round)
//   leaderboard_size
// ---------------------------------------------------------------------------
class RoundResultScreen : public Screen {
  bool _done = false;

 public:
  ScreenMode id()          const override { return SCREEN_RESULT; }
  bool       is_complete() const override { return _done; }

  void on_enter()    override { _done = false; }
  void on_button_a() override { _done = true; }
  void on_button_b() override { _done = true; }

  // Rank before this round. commit_round() already applied rank_delta, so
  // add it back to recover the previous value.
  int prev_rank() const {
    return session.player_rank + current_round.rank_delta;
  }

  void draw() override {
    display.fillScreen(WHITE);
    auto W = [](bool win) -> String { return win ? "W" : "L"; };
    draw_text_block("RESULT", 4, 4);
    draw_text_block(
      "maze:  " + format_ms(current_round.maze_player_time) +
      " vs " + format_ms(current_round.maze_opponent_time) +
      "  " + W(current_round.maze_player_time <= current_round.maze_opponent_time),
      4, 24);
    draw_text_block(
      "words: " + format_ms(current_round.words_player_time) +
      " vs " + format_ms(current_round.words_opponent_time) +
      "  " + W(current_round.words_player_time <= current_round.words_opponent_time),
      4, 40);
    draw_text_block(
      "count: " + format_ms(current_round.count_player_time) +
      " vs " + format_ms(current_round.count_opponent_time) +
      "  " + W(current_round.count_player_time <= current_round.count_opponent_time),
      4, 56);
    draw_text_block("score: " + String(current_round.player_wins) + "-" +
                    String(3 - current_round.player_wins), 4, 76);
    String arrow = (current_round.rank_delta > 0) ? "UP " : "DN ";
    draw_text_block(arrow + "#" + String(prev_rank()) + " -> #" +
                    String(session.player_rank) + "  (" +
                    String(current_round.rank_delta) + ")", 4, 92);
    draw_text_block("board: " + String(leaderboard_size), 4, 108);
    draw_text_block("A to continue", 4, SCREEN_HEIGHT - 16);
  }
};

#endif
