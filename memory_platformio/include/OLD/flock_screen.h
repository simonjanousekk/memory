#ifndef FLOCK_SCREEN_H
#define FLOCK_SCREEN_H

#include <Arduino.h>
#include <display.h>
#include <screen.h>

// ---------------------------------------------------------------------------
// FlockScreen — bare-bones boids for performance testing + live tuning.
//
// Naive O(N^2) Reynolds rules (cohesion / alignment / separation), single
// flock, wraparound edges, fillCircle boids. No spatial hashing yet — that's
// the next step once we know what the naive version costs.
//
// Controls:
//   encoder rotate  : navigate between stats (or change selected stat in edit)
//   button A        : respawn all boids at random
//   button B        : toggle edit mode on the currently-highlighted stat
// ---------------------------------------------------------------------------

class FlockScreen : public Screen {
public:
  static constexpr int MAX_BOIDS = 512;
  static constexpr int BOID_SIZE = 4;

  struct Boid {
    float x, y;
    float vx, vy;
  };

  // One tunable parameter, edited live via encoder + button B.
  struct Stat {
    const char *label;
    float value;
    float step;
    float minv;
    float maxv;
    int decimals;
  };

  enum {
    S_COUNT = 0,
    S_MAX_SPEED,
    S_MIN_SPEED,
    S_PERCEPTION,
    S_SEPARATION,
    S_W_COHESION,
    S_W_ALIGNMENT,
    S_W_SEPARATION,
    NUM_STATS,
  };

private:
  // Initial values match what we had as constexpr before.
  // {label, value, step, minv, maxv, decimals}
  Stat stats_[NUM_STATS] = {
      {"n", 100.0f, 10.0f, 10.0f, (float)MAX_BOIDS, 0},
      {"S", 2.0f, 0.1f, 0.1f, 10.0f, 1},
      {"s", 0.8f, 0.1f, 0.0f, 10.0f, 1},
      {"P", 30.0f, 2.0f, 5.0f, 200.0f, 0},
      {"R", 50.0f, 2.0f, 1.0f, 200.0f, 0},
      {"C", 0.002f, 0.001f, 0.0f, 0.1f, 3},
      {"A", 0.05f, 0.01f, 0.0f, 1.0f, 2},
      {"W", 0.2f, 0.01f, 0.0f, 1.0f, 2},
  };

  int selected_ = 0;
  bool editing_ = false;

  Boid boids_[MAX_BOIDS];
  unsigned long sim_us_ = 0;

  int count() const { return (int)stats_[S_COUNT].value; }

  void spawn_all() {
    float max_speed = stats_[S_MAX_SPEED].value;
    for (int i = 0; i < MAX_BOIDS; i++) {
      boids_[i].x = (float)(esp_random() % SCREEN_WIDTH);
      boids_[i].y = (float)(esp_random() % SCREEN_HEIGHT);
      float a = (float)(esp_random() % 6283) / 1000.0f; // 0..2π
      boids_[i].vx = cosf(a) * max_speed;
      boids_[i].vy = sinf(a) * max_speed;
    }
  }

public:
  ScreenMode id() const override { return SCREEN_FLOCK; }

  void on_init() override { spawn_all(); }
  void on_enter() override { spawn_all(); }

  void on_button_a() override { editing_ = !editing_; }
  void on_button_b() override { spawn_all(); }

  void on_encoder_rotate(int delta) override {
    if (editing_) {
      Stat &s = stats_[selected_];
      s.value += delta * s.step;
      if (s.value < s.minv)
        s.value = s.minv;
      if (s.value > s.maxv)
        s.value = s.maxv;
    } else {
      selected_ = ((selected_ + delta) % NUM_STATS + NUM_STATS) % NUM_STATS;
    }
  }

  void update() override {
    const int N = count();
    const float MAX_SP = stats_[S_MAX_SPEED].value;
    const float MIN_SP = stats_[S_MIN_SPEED].value;
    const float PER = stats_[S_PERCEPTION].value;
    const float SEP = stats_[S_SEPARATION].value;
    const float WC = stats_[S_W_COHESION].value;
    const float WA = stats_[S_W_ALIGNMENT].value;
    const float WS = stats_[S_W_SEPARATION].value;

    const float per2 = PER * PER;
    const float sep2 = SEP * SEP;
    const float max_sp2 = MAX_SP * MAX_SP;
    const float min_sp2 = MIN_SP * MIN_SP;

    unsigned long t0 = micros();

    for (int i = 0; i < N; i++) {
      Boid &b = boids_[i];

      float coh_x = 0, coh_y = 0;
      float ali_x = 0, ali_y = 0;
      float sep_x = 0, sep_y = 0;
      int neighbors = 0;

      for (int j = 0; j < N; j++) {
        if (j == i)
          continue;
        const Boid &o = boids_[j];
        float dx = o.x - b.x;
        float dy = o.y - b.y;
        float d2 = dx * dx + dy * dy;
        if (d2 > per2)
          continue;
        neighbors++;
        coh_x += o.x;
        coh_y += o.y;
        ali_x += o.vx;
        ali_y += o.vy;
        if (d2 < sep2 && d2 > 0.001f) {
          float inv = 1.0f / d2;
          sep_x -= dx * inv;
          sep_y -= dy * inv;
        }
      }

      if (neighbors > 0) {
        float inv_n = 1.0f / (float)neighbors;
        coh_x = coh_x * inv_n - b.x;
        coh_y = coh_y * inv_n - b.y;
        ali_x = ali_x * inv_n - b.vx;
        ali_y = ali_y * inv_n - b.vy;
        b.vx += coh_x * WC + ali_x * WA + sep_x * WS;
        b.vy += coh_y * WC + ali_y * WA + sep_y * WS;
      }

      float sp2 = b.vx * b.vx + b.vy * b.vy;
      if (sp2 > max_sp2) {
        float s = MAX_SP / sqrtf(sp2);
        b.vx *= s;
        b.vy *= s;
      } else if (sp2 < min_sp2) {
        if (sp2 < 0.0001f) {
          float a = (float)(esp_random() % 6283) / 1000.0f;
          b.vx = cosf(a) * MIN_SP;
          b.vy = sinf(a) * MIN_SP;
        } else {
          float s = MIN_SP / sqrtf(sp2);
          b.vx *= s;
          b.vy *= s;
        }
      }

      b.x += b.vx;
      b.y += b.vy;

      if (b.x < 0)
        b.x += SCREEN_WIDTH;
      else if (b.x >= SCREEN_WIDTH)
        b.x -= SCREEN_WIDTH;
      if (b.y < 0)
        b.y += SCREEN_HEIGHT;
      else if (b.y >= SCREEN_HEIGHT)
        b.y -= SCREEN_HEIGHT;
    }

    sim_us_ = micros() - t0;
  }

  void draw() override {
    const int N = count();
    for (int i = 0; i < N; i++) {
      int x = (int)boids_[i].x;
      int y = (int)boids_[i].y;
      display.fillCircle(x, y, BOID_SIZE, BLACK);
    }

    // Stats grid: 2 rows × 4 cols at top.
    char buf[24];
    const int cell_w = SCREEN_WIDTH / 4; // 100
    const int row_h = 20;
    for (int i = 0; i < NUM_STATS; i++) {
      const Stat &s = stats_[i];
      int sx = (i % 4) * cell_w + 1;
      int sy = (i / 4) * row_h + 1;
      bool sel = (i == selected_);
      bool edit = sel && editing_;
      if (edit)
        snprintf(buf, sizeof(buf), "[%s%.*f]", s.label, s.decimals, s.value);
      else
        snprintf(buf, sizeof(buf), "%s%.*f", s.label, s.decimals, s.value);
      draw_text_block(buf, sx, sy, sel ? WHITE : BLACK, sel, 2);
    }

    snprintf(buf, sizeof(buf), "sim=%.1fms%s", sim_us_ / 1000.0f,
             editing_ ? "  [EDIT]" : "");
    draw_text_block(buf, 1, SCREEN_HEIGHT - font_height * 2, BLACK);
  }
};

#endif
