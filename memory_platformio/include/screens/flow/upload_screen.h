#ifndef UPLOAD_SCREEN_H
#define UPLOAD_SCREEN_H

#include <WiFi.h>
#include <display.h>
#include <game_state.h>
#include <screen.h>
#include <supabase.h>

// ---------------------------------------------------------------------------
// UploadScreen
//
// Calls supabase_insert_player() once on the first update() tick.
// The HTTP POST is blocking (~1-3 s); the display holds the last drawn frame.
//
// Button B — retry on failure
// Button A — skip (mark complete without saving; rank is still shown)
//
// draw() is yours to design.
// Available variables:
//   _state          — UPLOAD_IDLE / UPLOAD_PENDING / UPLOAD_OK / UPLOAD_FAIL
//   _retry_count    — number of attempts made
//   session.player_name
//   session.player_rank
//   session.rounds_count
// ---------------------------------------------------------------------------
class UploadScreen : public Screen {
 public:
  enum UploadState { UPLOAD_IDLE, UPLOAD_PENDING, UPLOAD_OK, UPLOAD_FAIL };

 private:
  UploadState _state       = UPLOAD_IDLE;
  bool        _done        = false;
  int         _retry_count = 0;

  void _attempt() {
    _state = UPLOAD_PENDING;
    if (session.offline || WiFi.status() != WL_CONNECTED) {
      _state = UPLOAD_FAIL;
      _retry_count++;
      return;
    }
    GamePair send_buf[MAX_SESSION_ROUNDS];
    session.get_send_pairs(send_buf);
    bool ok = supabase_insert_player(
        session.player_name,
        session.player_rank,
        send_buf,
        (uint8_t)session.send_count());
    _state = ok ? UPLOAD_OK : UPLOAD_FAIL;
    if (!ok) _retry_count++;
    if (ok)  _done = true;
  }

 public:
  ScreenMode  id()          const override { return SCREEN_UPLOAD; }
  bool        is_complete() const override { return _done; }
  UploadState state()       const          { return _state; }

  void on_enter() override {
    _state       = UPLOAD_IDLE;
    _done        = false;
    _retry_count = 0;
  }

  void update() override {
    if (_state == UPLOAD_IDLE) _attempt();
  }

  void on_button_b() override {
    if (_state == UPLOAD_FAIL) { _state = UPLOAD_IDLE; }   // retry
  }

  void on_button_a() override { _done = true; }   // skip / continue anyway

  void draw() override {
    display.fillScreen(WHITE);
    draw_text_block("SAVING", 4, 4);
    draw_text_block(String("name:  ") + session.player_name, 4, 24);
    draw_text_block("rank:  #" + String(session.player_rank), 4, 40);
    draw_text_block("rounds: " + String(session.rounds_count), 4, 56);

    String status_str;
    switch (_state) {
      case UPLOAD_IDLE:    status_str = "idle...";   break;
      case UPLOAD_PENDING: status_str = "uploading..."; break;
      case UPLOAD_OK:      status_str = "saved!";    break;
      case UPLOAD_FAIL:    status_str = "FAILED  (retries: " + String(_retry_count) + ")"; break;
    }
    draw_text_block("status: " + status_str, 4, 80);

    if (_state == UPLOAD_FAIL)
      draw_text_block("B = retry  A = skip", 4, SCREEN_HEIGHT - 16);
  }
};

#endif
