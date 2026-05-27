#ifndef MAZE_SCREEN_H
#define MAZE_SCREEN_H

#include <background.h>
#include <game_seed.h>
#include <game_state.h>
#include <screen.h>
#include <screens/maze/dot.h>
#include <screens/maze/maze.h>
// ---------------------------------------------------------------------------
// MazeScreen
//
// Tilting-maze game: rotate the maze with the encoder, guide the dot through
// gravity and wall collisions out through the single exit at the bottom.
//
// Button A  — reset / new maze
// Encoder   — rotate maze (~12° per click)
// ---------------------------------------------------------------------------
class MazeScreen : public Screen {
  GameMaze maze;
  Dot dot;
  float _angle = 0.0f;
  TimedGame _timer;

  static constexpr float HW = GameMaze::cols * GameMaze::cell_px * 0.5f;
  static constexpr float HH = GameMaze::rows * GameMaze::cell_px * 0.5f;
  static constexpr float ROT_STEP = TWO_PI / 30.0f;  // radians per encoder click
  static constexpr float G = 400.0f;                 // gravity, px/s^2
  static constexpr float DT = 1.0f / 30.0f;          // physics tick = game loop rate
  static constexpr float DOT_R = 3.0f;               // visual radius, px
  static constexpr float COLLISION_PAD =
      2.0f;  // extra clearance from wall edge, px
  static constexpr float COLLISION_R = DOT_R + COLLISION_PAD;
  static constexpr int SUBSTEPS = 8;  // sub-steps per tick (anti-tunneling)

  // Check and resolve dot overlap with the walls of its current cell.
  // Called once per sub-step so displacement per check stays small.
  void resolve_walls() {
    const float C = (float)GameMaze::cell_px;
    const float cr = COLLISION_R;
    float x = dot.x(), y = dot.y();
    float vx = dot.vx(), vy = dot.vy();

    const float maze_bottom = GameMaze::rows * C;

    // Once the dot has passed the maze floor it is inside the exit chamber.
    // Enforce only the three chamber walls (left, right, floor) so that the
    // clamped-row logic below can never see the solid bottom border of an
    // adjacent column and push the dot back up out of the exit.
    if (y > maze_bottom) {
      const float ex0 = maze.exit_c() * C;
      const float ex1 = (maze.exit_c() + 1) * C;
      if (vx < 0 && x - cr < ex0) {
        x = ex0 + cr;
        vx = 0;
      }
      if (vx > 0 && x + cr > ex1) {
        x = ex1 - cr;
        vx = 0;
      }
      if (vy > 0 && y + cr > maze_bottom + C) {
        y = maze_bottom + C - cr;
        vy = 0;
      }
      dot.set_pos(x, y);
      dot.set_vel(vx, vy);
      return;
    }

    int col = constrain((int)(x / C), 0, GameMaze::cols - 1);
    int row = constrain((int)(y / C), 0, GameMaze::rows - 1);
    float cx = col * C, cy = row * C;

    if (vy < 0 && y - cr < cy && maze.has_wall(row, col, WALL_N)) {
      y = cy + cr;
      vy = 0;
    }
    if (vy > 0 && y + cr > cy + C && maze.has_wall(row, col, WALL_S)) {
      y = cy + C - cr;
      vy = 0;
    }
    if (vx < 0 && x - cr < cx && maze.has_wall(row, col, WALL_W)) {
      x = cx + cr;
      vx = 0;
    }
    if (vx > 0 && x + cr > cx + C && maze.has_wall(row, col, WALL_E)) {
      x = cx + C - cr;
      vx = 0;
    }

    dot.set_pos(x, y);
    dot.set_vel(vx, vy);
  }

 public:
  ScreenMode id() const override { return SCREEN_MAZE; }
  bool is_complete() const override { return _timer.won; }
  uint32_t finish_ms() const override { return _timer.finish_ms; }

  void on_enter() override {
    maze.generate(g_game_seed);
    dot.reset(HW, HH);
    _angle = 0.0f;
    _timer.begin();
  }

  void on_encoder_rotate(int delta) override { _angle += delta * ROT_STEP; }
  // void on_button_b() override { on_enter(); }

  void update() override {
    if (_timer.won)
      return;

    const float C = (float)GameMaze::cell_px;
    const float dt_sub = DT / SUBSTEPS;
    const float gx = G * sinf(_angle);  // gravity in maze-local X
    const float gy = G * cosf(_angle);  // gravity in maze-local Y

    for (int i = 0; i < SUBSTEPS; i++) {
      dot.add_vel(gx * dt_sub, gy * dt_sub);
      dot.step(dt_sub);
      resolve_walls();
    }

    dot.scale_vel(0.995f);  // light air friction

    // Win: dot fell through the exit gap at the bottom border
    int col = constrain((int)(dot.x() / C), 0, GameMaze::cols - 1);
    if (dot.y() > GameMaze::rows * C + C / 2 && col == maze.exit_c()) {
      _timer.complete();
    }
  }

  void draw() override {
    // display.fillScreen(BLACK);
    background.draw(RADIAL);
    maze.draw(_angle);
    dot.draw(_angle, HW, HH, (int)DOT_R);
  }
};

#endif
