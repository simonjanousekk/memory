#ifndef LOGO_SCREEN_H
#define LOGO_SCREEN_H

#include <WiFi.h>
#include <display.h>
#include <screen.h>
#include <sprites/logo_animation_sprites.h>
#include <sprites/logo_sprites.h>
#include <supabase.h>
#include <wifi/wifi_manager.h>

// Flag written by the FreeRTOS fetch task, read by the main loop.
// volatile so the compiler never caches it in a register.
static volatile bool _logo_fetch_done = false;

static void _logo_fetch_task(void* /*param*/) {
  supabase_fetch_leaderboard();  // writes leaderboard[] + leaderboard_size
  _logo_fetch_done = true;       // signal main loop; all writes are done
  vTaskDelete(NULL);
}

// ---------------------------------------------------------------------------
// LogoScreen
//
// Plays the logo animation while WiFi connects and the leaderboard is fetched
// in the background. Completes when the animation has played at least once
// AND the fetch has finished (success or failure).
//
// Any button press skips the wait and marks the screen complete immediately
// (leaderboard_size will be 0 → offline mode if fetch wasn't done yet).
//
// draw() is yours to design.
// Available variables:
//   animation_index          — current frame index (0 … logo_animation_allArray_LEN-1)
//   inverted                 — true when display is inverted
//   _anim_done               — true after first full animation cycle
//   _fetch_done              — true after supabase_fetch_leaderboard() returned
//   leaderboard_size         — global, 0 until fetch succeeds
// ---------------------------------------------------------------------------

class LogoScreen : public Screen {
  bool inverted = false;
  bool _complete = false;
  bool _anim_done = false;
  bool _fetch_done = false;
  bool _fetch_started = false;  // guard: only call fetch once

  int animation_index = 0;
  uint16_t animation_time_per_frame = 1000 / 30;
  uint32_t animation_last_time = 0;

  String dots = "";
  uint16_t dots_time_per_dot = 1000 / 2;
  uint32_t dots_last_time = 0;

  void
  _try_complete() {
    if (_anim_done && _fetch_done) _complete = true;
  }

 public:
  ScreenMode id() const override { return SCREEN_LOGO; }
  bool is_complete() const override { return _complete; }

  void on_enter() override {
    animation_index = 0;
    _complete = false;
    _anim_done = false;
    _fetch_done = false;
    _fetch_started = false;
    animation_last_time = millis();
  }

  void update() override {
    // Advance animation frame
    if (millis() - animation_last_time >= animation_time_per_frame) {
      animation_index++;
      if (animation_index >= logo_animation_allArray_LEN) {
        animation_index = 0;
        _anim_done = true;
      }
      animation_last_time = millis();
    }

    if (millis() - dots_last_time >= dots_time_per_dot) {
      add_dots();
      dots_last_time = millis();
    }

    // Kick off the fetch task once WiFi is connected (non-blocking).
    if (!_fetch_started && WiFi.status() == WL_CONNECTED) {
      _fetch_started = true;
      _logo_fetch_done = false;
      xTaskCreate(_logo_fetch_task, "lb_fetch",
          16384,  // stack — HTTPClient + ArduinoJson heap allocs need headroom
          nullptr, 1, nullptr);
    }

    // If WiFi has exhausted all networks and never connected, treat fetch as
    // resolved (leaderboard_size stays 0, game will run with random seeds).
    if (!_fetch_started && !_fetch_done && wifi_manager_failed()) {
      _fetch_done = true;
    }

    // Pick up the flag the task sets after writing leaderboard[].
    if (!_fetch_done && _logo_fetch_done) _fetch_done = true;

    _try_complete();
  }

  void draw() override {
    display.fillScreen(BLACK);
    display.drawBitmap(0, SCREEN_HEIGHT / 2 - 40, logo_animation_allArray[animation_index], 400, 68, WHITE, BLACK);
    // String status = _fetch_done ? "ready" : (_fetch_started ? "loading..." : "connecting...");
    String status = "loading" + dots;
    display.setFont(&Panell_Regular12pt7b);
    display.setTextColor(WHITE);
    display.setTextSize(1);
    display.setCursor(8, SCREEN_HEIGHT - 10);
    display.println(status);
  }

  void add_dots() {
    dots += ".";
    if (dots.length() > 3) {
      dots = "";
    }
  }

  // void on_button_a() override {
  //   inverted = !inverted;
  //   _complete = true;
  // }
  // void on_button_b() override { _complete = true; }
};

#endif
