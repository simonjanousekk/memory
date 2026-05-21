#ifndef GAME_SEED_H
#define GAME_SEED_H

#include <Arduino.h>

// Shared deterministic seed used by both MazeScreen and LettersScreen.
// Both screens call randomSeed(g_game_seed) before any random() calls, so the
// same seed always produces the same layout.  Change it in the debug menu.
uint32_t g_game_seed = esp_random();

#endif
