#ifndef TUNE_SCREEN_H
#define TUNE_SCREEN_H

#include <Arduino.h>
#include <display.h>
#include <screen.h>
#include <sprites/img_sprites.h>

#define TUNE_TILE_SIZE 8
#include <bitmap_scramble.h>

class TuneScreen : public Screen {
  int selected_value;
  int correct_value;
  int max_distance;
  int score;
  bool submitted;

  unsigned long _last_encoder_us = 0;
  int _encoder_step = 1;

  static int encoder_step_size(unsigned long dt_us, int &step) {
    constexpr unsigned long kIdleUs = 200000;
    constexpr unsigned long kFastUs = 35000;
    if (dt_us >= kIdleUs) {
      step = 1;
      return 1;
    }
    if (dt_us <= kFastUs)
      step = min(step + 1, 16);
    else
      step = max(1, step - 1);
    return step;
  }

public:
  ScreenMode id() const override { return SCREEN_TUNE; }

  void on_init() override {
    submitted = false;
    selected_value = SCREEN_WIDTH / 2;
    correct_value = esp_random() % SCREEN_WIDTH;
    max_distance = max(correct_value, SCREEN_WIDTH - correct_value);
    score = 0;
  }

  void draw() override {
    const int tile_count =
        (BB_WIDTH / TUNE_TILE_SIZE) * (BB_HEIGHT / TUNE_TILE_SIZE);
    const int distance = abs(selected_value - correct_value);
    const int swap_count = map(distance, 0, max_distance, 0, tile_count);
    draw_bitmap_scrambled(0, 0, DEV_DATA, BB_WIDTH, BB_HEIGHT, TUNE_TILE_SIZE,
                          swap_count, (uint32_t)correct_value, WHITE, BLACK);

    display.fillRect(0, SCREEN_HEIGHT - 20, SCREEN_WIDTH, 20, BLACK);
    // int x = map(selected_, 0, 1, 0, SCREEN_WIDTH);
    display.fillRect(selected_value - 1, SCREEN_HEIGHT - 20, 2, 20, WHITE);
    if (submitted) {
      display.fillRect(correct_value - 1, SCREEN_HEIGHT - 20, 2, 20, WHITE);
    }

    draw_text_block(String(score), 0, SCREEN_HEIGHT_HALF, WHITE);
  }

  void submit() {
    if (submitted)
      return;
    submitted = true;
    int distance = abs(selected_value - correct_value);
    score = map(distance, 0, SCREEN_WIDTH / 2, 100, 0);
  }

  void on_button_a() override { submit(); }
  void on_button_b() override { on_init(); }

  void on_encoder_rotate(int delta) override {
    if (delta == 0)
      return;

    const unsigned long now = micros();
    const unsigned long dt =
        (_last_encoder_us == 0) ? 200000UL : (now - _last_encoder_us);
    _last_encoder_us = now;

    const int ticks = abs(delta);
    const int dir = delta > 0 ? 1 : -1;
    // Fallback when ticks were queued during blocking SPI (rare after main
    // throttling).
    const unsigned long dt_per = max(1UL, dt / (unsigned long)ticks);
    const int step = encoder_step_size(dt_per, _encoder_step);
    selected_value += dir * step * ticks;

    if (selected_value < 0)
      selected_value = 0;
    if (selected_value > SCREEN_WIDTH)
      selected_value = SCREEN_WIDTH;
  }
};

#endif