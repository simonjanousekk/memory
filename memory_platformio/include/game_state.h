#ifndef GAME_STATE_H
#define GAME_STATE_H

#include <Arduino.h>

// Penalty added to elapsed time per wrong submission (milliseconds).
static constexpr uint32_t GAME_PENALTY_MS = 5000;

// ---------------------------------------------------------------------------
// TimedGame — shared timed-game bookkeeping for all three minigames.
//
// Usage:
//   on_enter()   → _timer.begin()
//   wrong answer → _timer.add_penalty()
//   correct/done → _timer.complete()
//   display      → _timer.elapsed(), _timer.format(), _timer.errors
// ---------------------------------------------------------------------------
struct TimedGame {
  uint32_t start_ms   = 0;
  uint32_t finish_ms  = 0;   // final time when complete (raw + penalties)
  uint32_t penalty_ms = 0;   // accumulated penalty
  uint8_t  errors     = 0;   // number of wrong submissions
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

  // Live elapsed time with penalties baked in (or frozen final time if won).
  uint32_t elapsed() const {
    if (won) return finish_ms;
    return (millis() - start_ms) + penalty_ms;
  }

  // "12.3s" — suitable for draw_text_block.
  String format() const {
    uint32_t ms = elapsed();
    return String(ms / 1000) + "." + String((ms % 1000) / 100) + "s";
  }
};

#endif
