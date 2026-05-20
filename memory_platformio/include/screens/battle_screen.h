#ifndef BATTLE_SCREEN_H
#define BATTLE_SCREEN_H

#include <Arduino.h>
#include <display.h>
#include <screen.h>

// ---------------------------------------------------------------------------
// BattleScreen — two flocks fight on a contained arena.
//
// Each side picks an archetype (preset of flocking + combat parameters).
// Boids attract toward enemies (aggression force) and resolve contacts with
// a density-weighted kill: the boid with fewer nearby allies is more likely
// to die. Tunable via COMBAT_RADIUS and ADVANTAGE_WEIGHT.
//
// Controls:
//   encoder rotate  : select side (A / B), or cycle archetype when editing
//   button B        : toggle edit mode on selected side
//   button A        : restart the battle with currently chosen archetypes
// ---------------------------------------------------------------------------

class BattleScreen : public Screen {
public:
  static constexpr int MAX_BOIDS = 400; // 200 per side max
  static constexpr int FLOCK_SIZE = 100;
  static constexpr int BOID_SIZE = 3; // drawing radius

  // Distance at which two enemies kill each other. Picked as ~2× BOID_SIZE so
  // visual circle-touching == death.
  static constexpr float DAMAGE_RADIUS = 6.0f;

  // Soft arena boundary — pushes boids away from screen edges rather than
  // wrapping. Battle on a torus would be visually confusing.
  static constexpr float BOUNDARY_MARGIN = 20.0f;
  static constexpr float BOUNDARY_FORCE = 0.4f;

  // Density-weighted combat: when two enemies touch, count nearby allies of
  // each. The one with fewer allies is more likely to die. ADVANTAGE_WEIGHT
  // controls how much numbers matter vs RNG:
  //   0.0 = pure coin flip (stats irrelevant)
  //   0.5 = pure stats (overwhelming side always wins)
  // 0.4 leaves a ~10% upset chance even at max disadvantage.
  static constexpr float COMBAT_RADIUS = 25.0f;
  static constexpr float ADVANTAGE_WEIGHT = 0.4f;

  struct Boid {
    float x, y;
    float vx, vy;
    uint8_t team;
    bool alive;
    bool will_die;
  };

  struct Archetype {
    const char *name;
    float w_cohesion;
    float w_alignment;
    float w_separation;
    float w_aggression; // steer toward nearest visible enemy
    float perception;
    float separation_r;
    float max_speed;
    float min_speed;
  };

  static const Archetype ARCHETYPES[];
  static const int NUM_ARCH;

private:
  Boid boids_[MAX_BOIDS];
  int arch_idx_[2] = {0, 1}; // index into ARCHETYPES for each side
  int selected_ = 0;         // 0 = A, 1 = B
  bool editing_ = false;
  int alive_count_[2] = {0, 0};
  unsigned long sim_us_ = 0;
  bool battle_over_ = false;
  int winner_ = -1; // -1 draw / not over, 0 = A, 1 = B

  void spawn_battle() {
    battle_over_ = false;
    winner_ = -1;
    const Archetype &arch_a = ARCHETYPES[arch_idx_[0]];
    const Archetype &arch_b = ARCHETYPES[arch_idx_[1]];

    for (int i = 0; i < FLOCK_SIZE; i++) {
      // Team A: spawned in the left third.
      boids_[i].x = (float)(esp_random() % (SCREEN_WIDTH / 3));
      boids_[i].y = (float)(esp_random() % SCREEN_HEIGHT);
      float a = (float)(esp_random() % 6283) / 1000.0f;
      boids_[i].vx = cosf(a) * arch_a.max_speed;
      boids_[i].vy = sinf(a) * arch_a.max_speed;
      boids_[i].team = 0;
      boids_[i].alive = true;
      boids_[i].will_die = false;
    }
    for (int i = 0; i < FLOCK_SIZE; i++) {
      int k = FLOCK_SIZE + i;
      // Team B: spawned in the right third.
      boids_[k].x =
          (float)(SCREEN_WIDTH * 2 / 3 + esp_random() % (SCREEN_WIDTH / 3));
      boids_[k].y = (float)(esp_random() % SCREEN_HEIGHT);
      float a = (float)(esp_random() % 6283) / 1000.0f;
      boids_[k].vx = cosf(a) * arch_b.max_speed;
      boids_[k].vy = sinf(a) * arch_b.max_speed;
      boids_[k].team = 1;
      boids_[k].alive = true;
      boids_[k].will_die = false;
    }
    alive_count_[0] = FLOCK_SIZE;
    alive_count_[1] = FLOCK_SIZE;
  }

  void apply_boundary(Boid &b) {
    if (b.x < BOUNDARY_MARGIN)
      b.vx += BOUNDARY_FORCE * (BOUNDARY_MARGIN - b.x) / BOUNDARY_MARGIN;
    else if (b.x > SCREEN_WIDTH - BOUNDARY_MARGIN)
      b.vx -= BOUNDARY_FORCE * (b.x - (SCREEN_WIDTH - BOUNDARY_MARGIN)) /
              BOUNDARY_MARGIN;
    if (b.y < BOUNDARY_MARGIN)
      b.vy += BOUNDARY_FORCE * (BOUNDARY_MARGIN - b.y) / BOUNDARY_MARGIN;
    else if (b.y > SCREEN_HEIGHT - BOUNDARY_MARGIN)
      b.vy -= BOUNDARY_FORCE * (b.y - (SCREEN_HEIGHT - BOUNDARY_MARGIN)) /
              BOUNDARY_MARGIN;
  }

  // Count alive allies within COMBAT_RADIUS of boid `idx` (excluding self and
  // anything already marked to die this tick).
  int count_local_allies(int idx) const {
    const Boid &b = boids_[idx];
    const float r2 = COMBAT_RADIUS * COMBAT_RADIUS;
    const int total = FLOCK_SIZE * 2;
    int n = 0;
    for (int j = 0; j < total; j++) {
      if (j == idx)
        continue;
      const Boid &o = boids_[j];
      if (!o.alive || o.will_die)
        continue;
      if (o.team != b.team)
        continue;
      float dx = o.x - b.x;
      float dy = o.y - b.y;
      if (dx * dx + dy * dy < r2)
        n++;
    }
    return n;
  }

public:
  ScreenMode id() const override { return SCREEN_BATTLE; }

  void on_init() override { spawn_battle(); }
  void on_enter() override { spawn_battle(); }

  void on_button_a() override { editing_ = !editing_; }
  void on_button_b() override { spawn_battle(); } // restart

  void on_encoder_rotate(int delta) override {
    if (editing_) {
      arch_idx_[selected_] =
          ((arch_idx_[selected_] + delta) % NUM_ARCH + NUM_ARCH) % NUM_ARCH;
    } else {
      selected_ = ((selected_ + delta) % 2 + 2) % 2;
    }
  }

  void update() override {
    if (battle_over_)
      return;

    const Archetype &arch_a = ARCHETYPES[arch_idx_[0]];
    const Archetype &arch_b = ARCHETYPES[arch_idx_[1]];
    const int total = FLOCK_SIZE * 2;

    unsigned long t0 = micros();

    // -------- Pass 1: forces + movement --------
    for (int i = 0; i < total; i++) {
      Boid &b = boids_[i];
      if (!b.alive)
        continue;

      const Archetype &arch = (b.team == 0) ? arch_a : arch_b;
      const float per2 = arch.perception * arch.perception;
      const float sep2 = arch.separation_r * arch.separation_r;
      const float max_sp2 = arch.max_speed * arch.max_speed;
      const float min_sp2 = arch.min_speed * arch.min_speed;

      float coh_x = 0, coh_y = 0;
      float ali_x = 0, ali_y = 0;
      float sep_x = 0, sep_y = 0;
      float agg_x = 0, agg_y = 0;
      int allies = 0;

      // Nearest enemy seeker for aggression.
      float nearest_e_d2 = per2;
      float nearest_e_dx = 0, nearest_e_dy = 0;
      bool has_enemy = false;

      for (int j = 0; j < total; j++) {
        if (j == i)
          continue;
        const Boid &o = boids_[j];
        if (!o.alive)
          continue;
        float dx = o.x - b.x;
        float dy = o.y - b.y;
        float d2 = dx * dx + dy * dy;

        if (o.team == b.team) {
          if (d2 > per2)
            continue;
          allies++;
          coh_x += o.x;
          coh_y += o.y;
          ali_x += o.vx;
          ali_y += o.vy;
          if (d2 < sep2 && d2 > 0.001f) {
            float inv = 1.0f / d2;
            sep_x -= dx * inv;
            sep_y -= dy * inv;
          }
        } else {
          // Enemy — track nearest for aggression.
          if (d2 < nearest_e_d2) {
            nearest_e_d2 = d2;
            nearest_e_dx = dx;
            nearest_e_dy = dy;
            has_enemy = true;
          }
        }
      }

      if (allies > 0) {
        float inv_n = 1.0f / (float)allies;
        coh_x = coh_x * inv_n - b.x;
        coh_y = coh_y * inv_n - b.y;
        ali_x = ali_x * inv_n - b.vx;
        ali_y = ali_y * inv_n - b.vy;
      } else {
        coh_x = coh_y = ali_x = ali_y = 0;
      }

      if (has_enemy) {
        float d = sqrtf(nearest_e_d2);
        if (d > 0.001f) {
          agg_x = nearest_e_dx / d;
          agg_y = nearest_e_dy / d;
        }
      }

      b.vx += coh_x * arch.w_cohesion + ali_x * arch.w_alignment +
              sep_x * arch.w_separation + agg_x * arch.w_aggression;
      b.vy += coh_y * arch.w_cohesion + ali_y * arch.w_alignment +
              sep_y * arch.w_separation + agg_y * arch.w_aggression;

      apply_boundary(b);

      float sp2 = b.vx * b.vx + b.vy * b.vy;
      if (sp2 > max_sp2) {
        float s = arch.max_speed / sqrtf(sp2);
        b.vx *= s;
        b.vy *= s;
      } else if (sp2 < min_sp2) {
        if (sp2 < 0.0001f) {
          float a = (float)(esp_random() % 6283) / 1000.0f;
          b.vx = cosf(a) * arch.min_speed;
          b.vy = sinf(a) * arch.min_speed;
        } else {
          float s = arch.min_speed / sqrtf(sp2);
          b.vx *= s;
          b.vy *= s;
        }
      }

      b.x += b.vx;
      b.y += b.vy;

      // Hard clamp in case the soft boundary loses a race.
      if (b.x < 0)
        b.x = 0;
      else if (b.x >= SCREEN_WIDTH)
        b.x = SCREEN_WIDTH - 1;
      if (b.y < 0)
        b.y = 0;
      else if (b.y >= SCREEN_HEIGHT)
        b.y = SCREEN_HEIGHT - 1;
    }

    // -------- Pass 2: combat resolution (density-weighted single kill) --------
    // For each contact, the boid with FEWER local allies is more likely to die.
    // One death per engagement, not two — the winner survives to fight later.
    const float dmg2 = DAMAGE_RADIUS * DAMAGE_RADIUS;
    for (int i = 0; i < total; i++) {
      if (!boids_[i].alive || boids_[i].will_die)
        continue;
      for (int j = i + 1; j < total; j++) {
        if (!boids_[j].alive || boids_[j].will_die)
          continue;
        if (boids_[i].team == boids_[j].team)
          continue;
        float dx = boids_[i].x - boids_[j].x;
        float dy = boids_[i].y - boids_[j].y;
        float d2 = dx * dx + dy * dy;
        if (d2 <= dmg2) {
          int ai = count_local_allies(i);
          int aj = count_local_allies(j);
          // advantage ∈ [-1, +1]; positive = i has more allies nearby.
          float advantage =
              (float)(ai - aj) / (float)(ai + aj + 1);
          float p_j_dies = 0.5f + ADVANTAGE_WEIGHT * advantage;
          float r = (float)esp_random() / (float)UINT32_MAX;
          if (r < p_j_dies)
            boids_[j].will_die = true; // i wins the exchange
          else
            boids_[i].will_die = true; // j wins the exchange
          break; // i has engaged; move on (regardless of who survived)
        }
      }
    }

    // -------- Pass 3: apply deaths, recount --------
    alive_count_[0] = 0;
    alive_count_[1] = 0;
    for (int i = 0; i < total; i++) {
      if (boids_[i].will_die) {
        boids_[i].alive = false;
        boids_[i].will_die = false;
      }
      if (boids_[i].alive)
        alive_count_[boids_[i].team]++;
    }

    if (alive_count_[0] == 0 || alive_count_[1] == 0) {
      battle_over_ = true;
      if (alive_count_[0] == 0 && alive_count_[1] == 0)
        winner_ = -1; // mutual annihilation
      else
        winner_ = (alive_count_[0] > 0) ? 0 : 1;
    }

    sim_us_ = micros() - t0;
  }

  void draw() override {
    const int total = FLOCK_SIZE * 2;
    for (int i = 0; i < total; i++) {
      if (!boids_[i].alive)
        continue;
      int x = (int)boids_[i].x;
      int y = (int)boids_[i].y;
      if (boids_[i].team == 0)
        display.fillCircle(x, y, BOID_SIZE, BLACK); // solid
      else
        display.drawCircle(x, y, BOID_SIZE, BLACK); // hollow
    }

    // Top: archetype selectors (2 cells of 200 px).
    char buf[32];
    const int cell_w = SCREEN_WIDTH / 2;
    for (int side = 0; side < 2; side++) {
      const Archetype &arch = ARCHETYPES[arch_idx_[side]];
      bool sel = (side == selected_);
      bool edit = sel && editing_;
      if (edit)
        snprintf(buf, sizeof(buf), "%c:[%s]", side == 0 ? 'A' : 'B', arch.name);
      else
        snprintf(buf, sizeof(buf), "%c: %s", side == 0 ? 'A' : 'B', arch.name);
      draw_text_block(buf, side * cell_w + 1, 1, sel ? WHITE : BLACK, sel, 2);
    }

    // Bottom: live counts + sim time, or winner banner.
    if (battle_over_) {
      const char *msg;
      if (winner_ == 0)
        msg = "A WINS  (A=restart)";
      else if (winner_ == 1)
        msg = "B WINS  (A=restart)";
      else
        msg = "DRAW   (A=restart)";
      draw_text_block(msg, 1, SCREEN_HEIGHT - font_height * 2, BLACK);
    } else {
      snprintf(buf, sizeof(buf), "A:%d  B:%d  sim=%.1fms%s", alive_count_[0],
               alive_count_[1], sim_us_ / 1000.0f, editing_ ? "  [EDIT]" : "");
      draw_text_block(buf, 1, SCREEN_HEIGHT - font_height * 2, BLACK);
    }
  }
};

// Definition of the static archetype table. Tweak / add entries here.
// {name, w_coh, w_ali, w_sep, w_agg, perception, sep_r, max_sp, min_sp}
const BattleScreen::Archetype BattleScreen::ARCHETYPES[] = {
    {"Balanced", 0.003f, 0.05f, 0.10f, 0.05f, 30.0f, 12.0f, 2.0f, 0.8f},
    {"Phalanx", 0.008f, 0.10f, 0.05f, 0.05f, 25.0f, 8.0f, 1.5f, 0.6f},
    {"Harasser", 0.001f, 0.04f, 0.15f, 0.08f, 40.0f, 20.0f, 3.0f, 1.5f},
    {"ScoutNet", 0.003f, 0.12f, 0.08f, 0.04f, 60.0f, 12.0f, 2.0f, 1.0f},
    {"Berserker", 0.001f, 0.02f, 0.05f, 0.25f, 25.0f, 8.0f, 2.5f, 1.2f},
    {"Swarm", 0.005f, 0.06f, 0.12f, 0.05f, 22.0f, 6.0f, 2.0f, 1.0f},
};
const int BattleScreen::NUM_ARCH =
    sizeof(BattleScreen::ARCHETYPES) / sizeof(BattleScreen::ARCHETYPES[0]);

#endif
