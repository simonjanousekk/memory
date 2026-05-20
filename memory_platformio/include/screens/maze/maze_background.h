#ifndef MAZE_BACKGROUND_H
#define MAZE_BACKGROUND_H

#include <Arduino.h>
#include <helpers.h>
#include <sprites/cell_sprites.h>

class MazeBackground {
  static constexpr int GRID_CELL_SIZE = 16;
  static constexpr int GRID_COLS = SCREEN_WIDTH / GRID_CELL_SIZE;
  static constexpr int GRID_ROWS = SCREEN_HEIGHT / GRID_CELL_SIZE;
  static constexpr int BUF_SIZE = SCREEN_WIDTH * SCREEN_HEIGHT / 8;

  // Pre-rendered 1-bit canvas — filled once, blitted every frame.
  GFXcanvas1 _canvas;

public:
  MazeBackground() : _canvas(SCREEN_WIDTH, SCREEN_HEIGHT) {}

  // Render all background tiles into the private canvas.
  // Call once on init (or whenever the background needs to change).
  void generate_grid() {
    _canvas.fillScreen(BLACK);

    const int half_cols = GRID_COLS / 2;
    const int half_rows = GRID_ROWS / 2;
    const float d_max = (float)(half_rows * half_rows + half_cols * half_cols);

    for (int r = 0; r < GRID_ROWS; r++) {
      for (int c = 0; c < GRID_COLS; c++) {
        const float d = dist_2(r, c, half_rows, half_cols);
        const int idx =
            constrain((int)map(d, 0, d_max, 0, cell_bayer_all_count - 1), 0,
                      cell_bayer_all_count - 1);
        _canvas.drawBitmap(c * GRID_CELL_SIZE, r * GRID_CELL_SIZE,
                           cell_bayer_all[idx], GRID_CELL_SIZE, GRID_CELL_SIZE,
                           WHITE, BLACK);
      }
    }
  }

  // Blit the pre-rendered buffer into the display canvas in one memcpy.
  void draw() { memcpy(display.getBuffer(), _canvas.getBuffer(), BUF_SIZE); }
};

#endif
