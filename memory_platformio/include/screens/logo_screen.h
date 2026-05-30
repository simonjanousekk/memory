#ifndef LOGO_SCREEN_H
#define LOGO_SCREEN_H

#include <WiFi.h>
#include <display.h>
#include <game_state.h>
#include <screen.h>
#include <sprites/logo_animation_sprites.h>
#include <sprites/logo_sprites.h>
#include <supabase.h>
#include <wifi/wifi_manager.h>

enum LogoMode { LOGO_BOOT, LOGO_SAVE };

static LogoMode _pending_logo_mode = LOGO_BOOT;

inline void logo_set_mode(LogoMode mode) { _pending_logo_mode = mode; }

// Flags written by FreeRTOS tasks, read by the main loop.
static volatile bool _logo_fetch_done = false;
static volatile bool _logo_upload_done = false;

static void _logo_fetch_task(void* /*param*/) {
  supabase_fetch_leaderboard();
  _logo_fetch_done = true;
  vTaskDelete(NULL);
}

static void _logo_save_task(void* /*param*/) {
  if (WiFi.status() == WL_CONNECTED && !session.offline) {
    GamePair send_buf[MAX_SESSION_ROUNDS];
    session.get_send_pairs(send_buf);
    supabase_insert_player(
        session.player_name,
        session.player_rank,
        send_buf,
        (uint8_t)session.send_count());
  }
  _logo_upload_done = true;

  supabase_fetch_leaderboard();
  _logo_fetch_done = true;
  vTaskDelete(NULL);
}

// ---------------------------------------------------------------------------
// LogoScreen
//
// LOGO_BOOT — plays animation while WiFi connects and leaderboard is fetched.
// LOGO_SAVE — after name entry: upload session, refresh leaderboard, then done.
//
// Completes when the animation has played at least once AND network work finished.
// ---------------------------------------------------------------------------
class LogoScreen : public Screen {
  LogoMode _mode = LOGO_BOOT;
  bool inverted = false;
  bool _complete = false;
  bool _anim_done = false;
  bool _fetch_done = false;
  bool _upload_done = false;
  bool _task_started = false;

  int animation_index = 0;
  uint16_t animation_time_per_frame = 1000 / 30;
  uint32_t animation_last_time = 0;

  String dots = "";
  uint16_t dots_time_per_dot = 1000 / 2;
  uint32_t dots_last_time = 0;

  void _try_complete() {
    if (_anim_done && _fetch_done) _complete = true;
  }

  void _start_task() {
    _task_started = true;
    _logo_fetch_done = false;
    _logo_upload_done = false;

    if (_mode == LOGO_SAVE) {
      xTaskCreate(_logo_save_task, "logo_save", 16384, nullptr, 1, nullptr);
    } else {
      xTaskCreate(_logo_fetch_task, "lb_fetch", 16384, nullptr, 1, nullptr);
    }
  }

 public:
  ScreenMode id() const override { return SCREEN_LOGO; }
  bool is_complete() const override { return _complete; }
  bool is_save_mode() const { return _mode == LOGO_SAVE; }

  void on_enter() override {
    _mode = _pending_logo_mode;
    animation_index = 0;
    _complete = false;
    _anim_done = false;
    _fetch_done = false;
    _upload_done = false;
    _task_started = false;
    animation_last_time = millis();
    dots = "";
    dots_last_time = millis();
  }

  void update() override {
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

    if (!_task_started && WiFi.status() == WL_CONNECTED) {
      _start_task();
    }

    if (!_task_started && !_fetch_done && wifi_manager_failed()) {
      _fetch_done = true;
      if (_mode == LOGO_SAVE) _upload_done = true;
    }

    if (_mode == LOGO_SAVE && !_upload_done && _logo_upload_done) {
      _upload_done = true;
    }

    if (!_fetch_done && _logo_fetch_done) _fetch_done = true;

    _try_complete();
  }

  void draw() override {
    display.fillScreen(BLACK);
    display.drawBitmap(0, SCREEN_HEIGHT / 2 - 40, logo_animation_allArray[animation_index], 400, 68, WHITE, BLACK);

    String status = (_mode == LOGO_SAVE && !_upload_done) ? "uploading" : "loading";
    status += dots;

    display.setFont(&Panell_Regular12pt7b);
    display.setTextColor(WHITE);
    display.setTextSize(1);
    display.setCursor(8, SCREEN_HEIGHT - 10);
    display.println(status);
  }

  void add_dots() {
    dots += ".";
    if (dots.length() > 3) dots = "";
  }
};

extern LogoScreen screen_logo;

#endif
