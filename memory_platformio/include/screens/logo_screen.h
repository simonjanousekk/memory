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

// Network I/O runs in a background task so the main loop keeps animating smoothly.
static volatile bool _logo_fetch_done = false;
static volatile bool _logo_upload_done = false;

static void _logo_fetch_task(void* /*param*/) {
  if (!supabase_fetch_leaderboard()) {
    Serial.println("[fetch] failed");
  }
  _logo_fetch_done = true;
  vTaskDelete(NULL);
}

static void _logo_save_task(void* /*param*/) {
  char name_copy[sizeof(session.player_name)];
  strncpy(name_copy, session.player_name, sizeof(name_copy) - 1);
  name_copy[sizeof(name_copy) - 1] = '\0';

  GamePair send_buf[MAX_SESSION_ROUNDS];
  int n = session.send_count();
  session.get_send_pairs(send_buf);

  if (WiFi.status() == WL_CONNECTED && n > 0 && name_copy[0] != '\0') {
    Serial.printf("[upload] name=%s rank=%d rounds=%d\n",
        name_copy, session.player_rank, n);
    if (!supabase_insert_player(
            name_copy, session.player_rank, send_buf, (uint8_t)n)) {
      Serial.println("[upload] insert failed");
    } else {
      Serial.println("[upload] done");
    }
  } else {
    Serial.printf("[upload] skipped wifi=%d rounds=%d name_empty=%d\n",
        WiFi.status() == WL_CONNECTED, n, name_copy[0] == '\0');
  }

  _logo_upload_done = true;

  // Brief pause after upload so TLS/JSON has heap for the large leaderboard GET.
  vTaskDelay(pdMS_TO_TICKS(300));

  if (!supabase_fetch_leaderboard()) {
    Serial.println("[fetch] failed after save");
  }
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

  void _start_net_task() {
    _task_started = true;
    _logo_fetch_done = false;
    _logo_upload_done = false;

    if (_mode == LOGO_SAVE) {
      xTaskCreate(_logo_save_task, "logo_save", 24576, nullptr, 1, nullptr);
    } else {
      xTaskCreate(_logo_fetch_task, "lb_fetch", 24576, nullptr, 1, nullptr);
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
    _logo_fetch_done = false;
    _logo_upload_done = false;
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
      _start_net_task();
    }

    if (!_task_started && !_fetch_done && wifi_manager_failed()) {
      _fetch_done = true;
      if (_mode == LOGO_SAVE) _upload_done = true;
    }

    if (_mode == LOGO_SAVE && !_upload_done && _logo_upload_done) {
      _upload_done = true;
    }

    if (!_fetch_done && _logo_fetch_done) {
      _fetch_done = true;
    }

    _try_complete();
  }

  void draw() override {
    display.fillScreen(BLACK);
    display.drawBitmap(0, SCREEN_HEIGHT / 2 - 40, logo_animation_allArray[animation_index], 400, 68, WHITE, BLACK);

    String status;
    if (_mode == LOGO_SAVE && !_task_started && !_fetch_done && !wifi_manager_failed()) {
      status = "connecting";
    } else if (_mode == LOGO_SAVE && !_upload_done) {
      status = "uploading";
    } else if (_mode == LOGO_SAVE) {
      status = "resetting";
    } else {
      status = "loading";
    }
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
