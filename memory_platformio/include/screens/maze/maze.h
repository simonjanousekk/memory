#ifndef MAZE_H
#define MAZE_H

#include <Arduino.h>
#include <helpers.h>

// ---------------------------------------------------------------------------
// Wall direction bitmask
//   Used by load() to accept external layouts. Internally walls are stored
//   in separate h/v arrays with zero redundancy (no shared-wall duplication).
// ---------------------------------------------------------------------------
static const uint8_t WALL_N = 0x1;
static const uint8_t WALL_E = 0x2;
static const uint8_t WALL_S = 0x4;
static const uint8_t WALL_W = 0x8;

// ---------------------------------------------------------------------------
// Maze<ROWS, COLS, CELL>
//
// Wall storage — each physical wall segment exists exactly once:
//   _h[ROWS+1][COLS]   horizontal wall above row r, column c
//                        r=0      → top border
//                        r=ROWS   → bottom border
//   _v[ROWS][COLS+1]   vertical wall left of column c, row r
//                        c=0      → left border
//                        c=COLS   → right border
//
// has_wall(r, c, dir) maps N/S/E/W to the canonical array entry.
// ---------------------------------------------------------------------------
template <int ROWS, int COLS, int CELL>
class Maze {
  bool _h[ROWS + 1][COLS];  // horizontal walls
  bool _v[ROWS][COLS + 1];  // vertical walls
  int _exit_r = ROWS - 1;
  int _exit_c = COLS / 2;
  uint32_t _last_seed = 0;

  // ---- Private draw helpers ------------------------------------------------

  // Wall half-thickness in local pixels. Kept as float so the perpendicular
  // offset is applied before rotation — giving consistent pixel width at every
  // angle instead of the ±1-pixel jitter you get from rounding first.
  static constexpr float WALL_T = 3.0f;

  void draw_segment(float x1, float y1, float x2, float y2, float hw, float hh,
      float ca, float sa, uint16_t color) const {
    // const float h = WALL_T * 0.5f;
    // if (y1 == y2) // horizontal wall: offset above/below in local space
    //   draw_rect(x1, y1 - h, x2 - x1, WALL_T, hw, hh, ca, sa, color);
    // else // vertical wall (x1 == x2): offset left/right in local space
    //   draw_rect(x1 - h, y1, WALL_T, y2 - y1, hw, hh, ca, sa, color);
    // display.fillCircle(x1, y1, WALL_T, color);
    // display.fillCircle(x2, y2, WALL_T, color);

    int sx1, sy1, sx2, sy2;
    rotate_to_screen(x1, y1, hw, hh, ca, sa, sx1, sy1);
    rotate_to_screen(x2, y2, hw, hh, ca, sa, sx2, sy2);
    display.drawLine(sx1, sy1, sx2, sy2, color);
  }

  // Draws a rotated filled quad (two triangles) in maze-local space.
  void draw_rect(float x, float y, float w, float h, float hw, float hh,
      float ca, float sa, uint16_t color) const {
    int sx1, sy1, sx2, sy2, sx3, sy3, sx4, sy4;
    rotate_to_screen(x, y, hw, hh, ca, sa, sx1, sy1);
    rotate_to_screen(x + w, y, hw, hh, ca, sa, sx2, sy2);
    rotate_to_screen(x, y + h, hw, hh, ca, sa, sx3, sy3);
    rotate_to_screen(x + w, y + h, hw, hh, ca, sa, sx4, sy4);
    display.fillTriangle(sx1, sy1, sx2, sy2, sx3, sy3, color);
    display.fillTriangle(sx2, sy2, sx3, sy3, sx4, sy4, color);
  }

 public:
  static constexpr int rows = ROWS;
  static constexpr int cols = COLS;
  static constexpr int cell_px = CELL;

  // ---- Layout methods -------------------------------------------------------

  // Iterative randomized DFS (depth-first backtracker).
  //   seed = 0  → use hardware RNG (esp_random); otherwise deterministic.
  // Produces a perfect maze (exactly one path between any two cells).
  // Carving starts at the center so the dot always begins in open space.
  // The exit gap is cut at the middle of the bottom border.
  void generate(uint32_t seed = 0) {
    _last_seed = seed ? seed : esp_random();
    randomSeed(_last_seed);

    // All walls solid (borders + interiors); DFS will carve openings.
    memset(_h, 0xFF, sizeof(_h));
    memset(_v, 0xFF, sizeof(_v));

    bool visited[ROWS][COLS];
    memset(visited, 0, sizeof(visited));

    struct Cell {
      int8_t r, c;
    };
    Cell stack[ROWS * COLS];
    int top = 0;

    const int dr[4] = {-1, 0, +1, 0};  // N E S W
    const int dc[4] = {0, +1, 0, -1};

    const int sr = ROWS / 2, sc = COLS / 2;
    visited[sr][sc] = true;
    stack[top++] = {(int8_t)sr, (int8_t)sc};

    while (top > 0) {
      const int r = stack[top - 1].r;
      const int c = stack[top - 1].c;

      int dirs[4], nd = 0;
      for (int d = 0; d < 4; d++) {
        int nr = r + dr[d], nc = c + dc[d];
        if (nr >= 0 && nr < ROWS && nc >= 0 && nc < COLS && !visited[nr][nc])
          dirs[nd++] = d;
      }

      if (nd == 0) {
        top--;
        continue;
      }

      const int d = dirs[random(nd)];
      const int nr = r + dr[d], nc = c + dc[d];

      switch (d) {
        case 0:
          _h[r][c] = false;
          break;  // N
        case 1:
          _v[r][c + 1] = false;
          break;  // E
        case 2:
          _h[r + 1][c] = false;
          break;  // S
        case 3:
          _v[r][c] = false;
          break;  // W
      }

      visited[nr][nc] = true;
      stack[top++] = {(int8_t)nr, (int8_t)nc};
    }

    // Exit gap at bottom-center.
    _exit_r = ROWS - 1;
    _exit_c = COLS / 2;
    _h[ROWS][_exit_c] = false;
  }

  uint32_t last_seed() const { return _last_seed; }

  // ---- Queries --------------------------------------------------------------

  bool has_wall(int r, int c, uint8_t dir) const {
    if (r < 0 || r >= ROWS || c < 0 || c >= COLS)
      return false;
    switch (dir) {
      case WALL_N:
        return _h[r][c];
      case WALL_S:
        return _h[r + 1][c];
      case WALL_W:
        return _v[r][c];
      case WALL_E:
        return _v[r][c + 1];
    }
    return false;
  }

  int exit_r() const { return _exit_r; }
  int exit_c() const { return _exit_c; }

  // ---- Draw -----------------------------------------------------------------

  void draw(float angle) const {
    const float hw = COLS * CELL * 0.5f;
    const float hh = ROWS * CELL * 0.5f;
    const float ca = cosf(angle), sa = sinf(angle);

    // White background slightly larger than the maze
    const int pad = 5;
    draw_rect(-pad, -pad, hw * 2 + pad * 2, hh * 2 + pad * 2, hw, hh, ca, sa,
        WHITE);

    // Exit chamber background: 1 cell wide (matching the exit gap), 1 cell
    // tall.
    const float ecx0 = (float)(_exit_c * CELL);
    const float ecx1 = (float)((_exit_c + 1) * CELL);
    const float ecy0 = (float)(ROWS * CELL);
    const float ecy1 = (float)((ROWS + 1) * CELL);
    draw_rect(ecx0 - pad, ecy0 - pad, ecx1 - ecx0 + pad * 2,
        ecy1 - ecy0 + pad * 2, hw, hh, ca, sa, WHITE);

    for (int r = 0; r <= ROWS; r++)
      for (int c = 0; c < COLS; c++)
        if (_h[r][c])
          draw_segment((float)(c * CELL), (float)(r * CELL),
              (float)((c + 1) * CELL), (float)(r * CELL), hw, hh,
              ca, sa, BLACK);

    for (int r = 0; r < ROWS; r++)
      for (int c = 0; c <= COLS; c++)
        if (_v[r][c])
          draw_segment((float)(c * CELL), (float)(r * CELL), (float)(c * CELL),
              (float)((r + 1) * CELL), hw, hh, ca, sa, BLACK);

    // Exit chamber walls: left, right, and bottom sides.
    // The top is already formed by the maze's bottom border (with the exit
    // gap).
    draw_segment(ecx0, ecy0, ecx0, ecy1, hw, hh, ca, sa, BLACK);  // left
    draw_segment(ecx1, ecy0, ecx1, ecy1, hw, hh, ca, sa, BLACK);  // right
    draw_segment(ecx0, ecy1, ecx1, ecy1, hw, hh, ca, sa, BLACK);  // bottom
  }

  // void draw(float angle) const {
  //   const float hw = COLS * CELL * 0.5f;
  //   const float hh = ROWS * CELL * 0.5f;
  //   const float ca = cosf(angle), sa = sinf(angle);

  //   const int WT = 4;
  //   const int HWT = WT * 0.5f;

  //   const int pad = 5;

  //   // draw_rect(0, 0, COLS * CELL, ROWS * CELL, hw, hh, ca, sa, BLACK);
  //   draw_rect(-pad, -pad, hw * 2 + pad * 2, hh * 2 + pad * 2, hw, hh, ca, sa,
  //             BLACK);

  //   for (int r = 0; r < ROWS; r++) {
  //     for (int c = 0; c < COLS; c++) {
  //       int x = c * CELL;
  //       int y = r * CELL;
  //       draw_rect(x + HWT, y + HWT, CELL - WT, CELL - WT, hw, hh, ca, sa,
  //                 WHITE);

  //       if (!has_wall(r, c, WALL_N)) {
  //         draw_rect(x + HWT, y, CELL - WT, HWT, hw, hh, ca, sa, WHITE);
  //       }
  //       if (!has_wall(r, c, WALL_E)) {
  //         draw_rect(x + CELL - HWT, y + HWT, HWT, CELL - WT, hw, hh, ca, sa,
  //                   WHITE);
  //       }
  //       if (!has_wall(r, c, WALL_S)) {
  //         draw_rect(x + HWT, y + CELL - HWT, CELL - WT, HWT, hw, hh, ca, sa,
  //                   WHITE);
  //       }
  //       if (!has_wall(r, c, WALL_W)) {
  //         draw_rect(x, y + HWT, HWT, CELL - WT, hw, hh, ca, sa, WHITE);
  //       }
  //     }
  //   }
  // }
};

// ---------------------------------------------------------------------------
// Change these three numbers to resize the maze. Everything else adapts.
// ---------------------------------------------------------------------------
using GameMaze = Maze<9, 9, 16>;

#endif
