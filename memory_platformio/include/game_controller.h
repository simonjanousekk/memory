#ifndef GAME_CONTROLLER_H
#define GAME_CONTROLLER_H

#include <WiFi.h>
#include <game_seed.h>
#include <game_state.h>
#include <screen.h>
#include <screens/logo_screen.h>

// ---------------------------------------------------------------------------
// GamePhase — one value per step in the game flow.
// ---------------------------------------------------------------------------
enum GamePhase {
  PHASE_LOGO,    // plays animation + fetches leaderboard in background
  PHASE_INTRO,
  PHASE_OPPONENT,
  PHASE_MAZE,
  PHASE_LETTERS,
  PHASE_COUNT,
  PHASE_RESULT,
  PHASE_GO_AGAIN,
  PHASE_NAME_ENTRY,
};

// Maps each phase to the ScreenMode that should be active during it.
static ScreenMode _phase_screen(GamePhase p) {
  switch (p) {
    case PHASE_LOGO:         return SCREEN_LOGO;
    case PHASE_INTRO:        return SCREEN_INTRO;
    case PHASE_OPPONENT:     return SCREEN_OPPONENT;
    case PHASE_MAZE:         return SCREEN_MAZE;
    case PHASE_LETTERS:      return SCREEN_LETTERS;
    case PHASE_COUNT:        return SCREEN_COUNT;
    case PHASE_RESULT:       return SCREEN_RESULT;
    case PHASE_GO_AGAIN:     return SCREEN_GO_AGAIN;
    case PHASE_NAME_ENTRY:   return SCREEN_NAME_ENTRY;
  }
  return SCREEN_LOGO;
}

// ---------------------------------------------------------------------------
// GameController
//
// Call start() once from setup(). Call update() every game tick (inside the
// 30 Hz game-update block in loop()).
//
// advance(next) — sets the phase, seeds g_game_seed for minigames, and
//                 calls set_screen() to swap the active screen.
//
// update() — polls currentScreen->is_complete(). When true, records any
//             minigame results, mutates session/current_round as needed,
//             then advances to the next phase.  Does nothing if:
//               • the debug menu is open, or
//               • the current screen doesn't match the expected phase
//                 (means a debug-menu nav put us on a different screen).
// ---------------------------------------------------------------------------
struct GameController {
  GamePhase phase           = PHASE_LOGO;
  bool      _tutorial_done = false;  // true after the first solo run

  void start() {
    logo_set_mode(LOGO_BOOT);
    advance(PHASE_LOGO);
  }

  void advance(GamePhase next) {
    phase = next;

    // Set the shared seed before entering any minigame screen.
    switch (next) {
      case PHASE_MAZE:    g_game_seed = current_round.maze_seed;   break;
      case PHASE_LETTERS: g_game_seed = current_round.words_seed;  break;
      case PHASE_COUNT:   g_game_seed = current_round.count_seed;  break;
      default: break;
    }

    set_screen(_phase_screen(next));
  }

  void update() {
    // Debug menu takes priority — never interfere with it.
    if (currentScreen->id() == SCREEN_DEBUG_MENU) return;

    // Guard: only act when the screen matches what this phase expects.
    // This prevents the controller from reacting to screens opened via
    // debug-menu navigation that bypass advance().
    if (currentScreen->id() != _phase_screen(phase)) return;

    if (!currentScreen->is_complete()) return;

    switch (phase) {

      // ---- Boot (logo plays + leaderboard fetched inside LogoScreen) --------
      case PHASE_LOGO:
        if (screen_logo.is_save_mode()) {
          logo_set_mode(LOGO_BOOT);
          advance(PHASE_LOGO);
        } else {
          session.offline = (WiFi.status() != WL_CONNECTED);
          session.begin(leaderboard_size > 0 ? leaderboard_size / 2 + 1 : 1);
          _tutorial_done = false;
          advance(PHASE_INTRO);
        }
        break;

      // ---- Session start ---------------------------------------------------
      case PHASE_INTRO:
        if (!_tutorial_done) {
          // First run: solo with random seeds — populates DB with fresh seeds.
          current_round.opponent_idx        = -1;
          current_round.maze_seed           = esp_random();
          current_round.maze_opponent_time  = UINT32_MAX;
          current_round.words_seed          = esp_random();
          current_round.words_opponent_time = UINT32_MAX;
          current_round.count_seed          = esp_random();
          current_round.count_opponent_time = UINT32_MAX;
          advance(PHASE_MAZE);   // solo: skip opponent screen
        } else {
          advance(PHASE_OPPONENT);
        }
        break;

      // ---- Round setup (OpponentScreen picks seeds) -----------------------
      case PHASE_OPPONENT:
        advance(PHASE_MAZE);
        break;

      // ---- Minigames -------------------------------------------------------
      case PHASE_MAZE:
        current_round.maze_player_time = currentScreen->finish_ms();
        advance(PHASE_LETTERS);
        break;

      case PHASE_LETTERS:
        current_round.words_player_time = currentScreen->finish_ms();
        advance(PHASE_COUNT);
        break;

      case PHASE_COUNT:
        current_round.count_player_time = currentScreen->finish_ms();
        if (!_tutorial_done) {
          // First run done: store pair (no rank change), then start battles.
          session.record_tutorial_pair(current_round);
          _tutorial_done = true;
          advance(PHASE_OPPONENT);
        } else {
          current_round.compute();
          session.commit_round(current_round, leaderboard_size);
          advance(PHASE_RESULT);
        }
        break;

      // ---- Round result + decision -----------------------------------------
      case PHASE_RESULT:
        advance(PHASE_GO_AGAIN);
        break;

      case PHASE_GO_AGAIN:
        // complete_result() == 1 → GO AGAIN; 0 → KEEP (go to name entry)
        if (currentScreen->complete_result() == 1)
          advance(PHASE_OPPONENT);
        else
          advance(PHASE_NAME_ENTRY);
        break;

      // ---- Save + farewell -------------------------------------------------
      case PHASE_NAME_ENTRY:
        logo_set_mode(LOGO_SAVE);
        advance(PHASE_LOGO);
        break;
    }
  }
};

GameController game_controller;

#endif
