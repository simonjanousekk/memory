#ifndef GAME_STATE_H
#define GAME_STATE_H

#include <Arduino.h>

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------
static constexpr uint32_t GAME_PENALTY_MS      = 5000;
static constexpr int      MAX_PLAYERS          = 100;
static constexpr uint8_t  MAX_GAMES_PER_PLAYER = 20;
// Circular-buffer capacity: the player can play unlimited rounds, but only
// the most recent MAX_SESSION_ROUNDS are kept in RAM and sent to Supabase.
static constexpr int      MAX_SESSION_ROUNDS   = 20;

// ---------------------------------------------------------------------------
// TimedGame — per-minigame timer with penalty support.
//
//   on_enter()   → begin()
//   wrong submit → add_penalty()
//   finished     → complete()
//   read back    → won, finish_ms, elapsed(), format()
// ---------------------------------------------------------------------------
struct TimedGame {
  uint32_t start_ms   = 0;
  uint32_t finish_ms  = 0;
  uint32_t penalty_ms = 0;
  uint8_t  errors     = 0;
  bool     won        = false;

  void begin() {
    start_ms   = millis();
    finish_ms  = 0;
    penalty_ms = 0;
    errors     = 0;
    won        = false;
  }

  void add_penalty() {
    penalty_ms += GAME_PENALTY_MS;
    errors++;
  }

  void complete() {
    won       = true;
    finish_ms = (millis() - start_ms) + penalty_ms;
  }

  uint32_t elapsed() const {
    if (won) return finish_ms;
    return (millis() - start_ms) + penalty_ms;
  }

  // "12.3s"
  String format() const {
    uint32_t ms = elapsed();
    return String(ms / 1000) + "." + String((ms % 1000) / 100) + "s";
  }
};

// Format raw ms for display. UINT32_MAX = no reference time → "---".
static inline String format_ms(uint32_t ms) {
  if (ms == UINT32_MAX) return "---";
  return String(ms / 1000) + "." + String((ms % 1000) / 100) + "s";
}

// ---------------------------------------------------------------------------
// GamePair — seeds + times for one complete round (all 3 minigames).
// This is the unit stored per-player in Supabase.
// ---------------------------------------------------------------------------
struct GamePair {
  uint32_t maze_seed  = 0, maze_time  = 0;
  uint32_t words_seed = 0, words_time = 0;
  uint32_t count_seed = 0, count_time = 0;
};

// ---------------------------------------------------------------------------
// LeaderboardEntry — one player row fetched from Supabase.
// ---------------------------------------------------------------------------
struct LeaderboardEntry {
  char     name[16] = {};
  int      rank     = 0;
  GamePair games[MAX_GAMES_PER_PLAYER];
  uint8_t  games_count = 0;
  bool     is_ghost    = false;
};

// ---------------------------------------------------------------------------
// RoundState — mutable state for the round currently in progress.
// Seeds come from the opponent's collection; each minigame picks independently.
// ---------------------------------------------------------------------------
struct RoundState {
  int opponent_idx = -1;

  uint32_t maze_seed  = 0, maze_opponent_time  = 0, maze_player_time  = 0;
  uint32_t words_seed = 0, words_opponent_time = 0, words_player_time = 0;
  uint32_t count_seed = 0, count_opponent_time = 0, count_player_time = 0;

  uint8_t player_wins = 0;
  int     rank_delta  = 0;

  // Call after all 3 minigames are done.
  void compute() {
    player_wins = 0;
    if (maze_player_time  < maze_opponent_time)  player_wins++;
    if (words_player_time < words_opponent_time) player_wins++;
    if (count_player_time < count_opponent_time) player_wins++;
    rank_delta = (int)(2 * player_wins) - 3;  // -3, -1, +1, or +3
  }

  // Flatten into a GamePair to append to session.rounds[].
  GamePair to_game_pair() const {
    GamePair gp;
    gp.maze_seed  = maze_seed;   gp.maze_time  = maze_player_time;
    gp.words_seed = words_seed;  gp.words_time = words_player_time;
    gp.count_seed = count_seed;  gp.count_time = count_player_time;
    return gp;
  }
};

// ---------------------------------------------------------------------------
// Session — state for the whole player session.
//
// rounds[] is a circular buffer of capacity MAX_SESSION_ROUNDS.
// rounds_count is unbounded — it counts every round ever played this session.
// Only the most recent min(rounds_count, MAX_SESSION_ROUNDS) are kept.
// ---------------------------------------------------------------------------
struct Session {
  int      player_rank  = 1;
  char     player_name[8] = {};  // enforced by NameEntryScreen (5 chars max)
  GamePair rounds[MAX_SESSION_ROUNDS];
  int      rounds_count = 0;     // total rounds played, may exceed buffer size
  bool     offline      = false;

  void begin(int starting_rank) {
    player_rank    = starting_rank;
    player_name[0] = '\0';
    rounds_count   = 0;
    offline        = false;
  }

  // Store the solo tutorial game pair without touching the rank.
  // Called after the first solo run before the normal battle loop begins.
  void record_tutorial_pair(const RoundState& r) {
    rounds[rounds_count % MAX_SESSION_ROUNDS] = r.to_game_pair();
    rounds_count++;
  }

  void commit_round(const RoundState& r, int board_size = 0) {
    rounds[rounds_count % MAX_SESSION_ROUNDS] = r.to_game_pair();
    rounds_count++;
    int new_rank = player_rank - r.rank_delta;
    if (new_rank < 1) new_rank = 1;
    if (board_size > 0 && new_rank > board_size + 1)
      new_rank = board_size + 1;
    player_rank = new_rank;
  }

  // Number of entries that will be sent to Supabase.
  int send_count() const {
    return min(rounds_count, MAX_SESSION_ROUNDS);
  }

  // Fill out_buf with the latest send_count() pairs in chronological order.
  // out_buf must have capacity >= send_count().
  void get_send_pairs(GamePair* out_buf) const {
    int n     = send_count();
    int start = rounds_count - n;  // absolute index of the oldest to include
    for (int i = 0; i < n; i++) {
      out_buf[i] = rounds[(start + i) % MAX_SESSION_ROUNDS];
    }
  }
};

// ---------------------------------------------------------------------------
// Globals — defined once here, shared across all includes.
// ---------------------------------------------------------------------------
LeaderboardEntry leaderboard[MAX_PLAYERS];
int              leaderboard_size = 0;
Session          session;
RoundState       current_round;

#endif
