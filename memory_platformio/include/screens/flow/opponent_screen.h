#ifndef OPPONENT_SCREEN_H
#define OPPONENT_SCREEN_H

#include <display.h>
#include <game_seed.h>
#include <game_state.h>
#include <screen.h>

// ---------------------------------------------------------------------------
// OpponentScreen
//
// Selects the opponent (one rank above the player's current rank) and picks
// seeds independently for each of the three minigames from that opponent's
// game collection.  Results are written into the global current_round.
//
// If the opponent has no games (or the board is empty / offline), random seeds
// are generated and opponent times are set to UINT32_MAX so the player always
// "wins" each minigame comparison — a graceful offline fallback.
//
// Press A to accept and proceed to the first tutorial.
//
// draw() is yours to design.
// Available variables:
//   _opponent_name[16]           — opponent's name
//   _opponent_rank               — opponent's rank number
//   session.player_rank          — current player rank
//   session.rounds_count         — rounds played so far this session
//   leaderboard_size             — total players
//   current_round.maze_opponent_time   — opponent's maze time (ms)
//   current_round.words_opponent_time  — opponent's letters time (ms)
//   current_round.count_opponent_time  — opponent's count time (ms)
// ---------------------------------------------------------------------------
class OpponentScreen : public Screen {
  bool _done               = false;
  char _opponent_name[16]  = {};
  int  _opponent_rank      = 0;

 public:
  ScreenMode id()          const override { return SCREEN_OPPONENT; }
  bool       is_complete() const override { return _done; }

  void on_enter() override {
    _done = false;

    // Determine opponent index (0-based into leaderboard[]).
    // Fight the player one rank above; at rank 1 fight rank 2 instead.
    int opp_idx = (session.player_rank <= 1)
                  ? 1
                  : session.player_rank - 2;
    opp_idx = constrain(opp_idx, 0, leaderboard_size - 1);

    if (leaderboard_size > 0) {
      LeaderboardEntry& opp = leaderboard[opp_idx];
      strncpy(_opponent_name, opp.name, sizeof(_opponent_name) - 1);
      _opponent_name[sizeof(_opponent_name) - 1] = '\0';
      _opponent_rank             = opp.rank;
      current_round.opponent_idx = opp_idx;

      if (opp.games_count > 0) {
        // Each minigame picks its seed independently from the opponent's pool.
        GamePair& ms = opp.games[random(opp.games_count)];
        GamePair& ws = opp.games[random(opp.games_count)];
        GamePair& cs = opp.games[random(opp.games_count)];
        current_round.maze_seed           = ms.maze_seed;
        current_round.maze_opponent_time  = ms.maze_time;
        current_round.words_seed          = ws.words_seed;
        current_round.words_opponent_time = ws.words_time;
        current_round.count_seed          = cs.count_seed;
        current_round.count_opponent_time = cs.count_time;
        return;
      }
    }

    // Offline / no games: random seeds, player always wins comparisons.
    strncpy(_opponent_name, "???", sizeof(_opponent_name));
    _opponent_rank                    = 0;
    current_round.opponent_idx        = -1;
    current_round.maze_seed           = esp_random();
    current_round.maze_opponent_time  = UINT32_MAX;
    current_round.words_seed          = esp_random();
    current_round.words_opponent_time = UINT32_MAX;
    current_round.count_seed          = esp_random();
    current_round.count_opponent_time = UINT32_MAX;
  }

  void on_button_a() override { _done = true; }
  void on_button_b() override { _done = true; }

  void draw() override {
    display.fillScreen(WHITE);
    draw_text_block("OPPONENT", 4, 4);
    draw_text_block("your rank: #" + String(session.player_rank), 4, 20);
    draw_text_block(String("vs  ") + _opponent_name + "  #" + String(_opponent_rank), 4, 36);
    draw_text_block("maze:  " + format_ms(current_round.maze_opponent_time), 4, 60);
    draw_text_block("words: " + format_ms(current_round.words_opponent_time), 4, 76);
    draw_text_block("count: " + format_ms(current_round.count_opponent_time), 4, 92);
    draw_text_block("round " + String(session.rounds_count + 1), 4, 116);
    draw_text_block("A to challenge", 4, SCREEN_HEIGHT - 16);
  }
};

#endif
