#ifndef BACKGROUND_H
#define BACKGROUND_H

#include <Arduino.h>
#include <display.h>
#include <helpers.h>
#include <sprites/cell_sprites.h>

enum BackgroundType {
  RADIAL,
  SOLID,
  RANDOM,
};

class Background {
  static constexpr int GRID_CELL_SIZE = 16;
  static constexpr int GRID_COLS = SCREEN_WIDTH / GRID_CELL_SIZE;
  static constexpr int GRID_ROWS = SCREEN_HEIGHT / GRID_CELL_SIZE;
  static constexpr int BUF_SIZE = SCREEN_WIDTH * SCREEN_HEIGHT / 8;

  // Pre-rendered 1-bit canvas — filled once, blitted every frame.
  GFXcanvas1 _canvas_radial;
  GFXcanvas1 _canvas_solid;
  GFXcanvas1 _canvas_random;

 public:
  Background() : _canvas_radial(SCREEN_WIDTH, SCREEN_HEIGHT),
                 _canvas_solid(SCREEN_WIDTH, SCREEN_HEIGHT),
                 _canvas_random(SCREEN_WIDTH, SCREEN_HEIGHT) {
    generate_background();
  }

  // Render all background tiles into the private canvas.
  // Call once on init (or whenever the background needs to change).
  void generate_background() {
    _canvas_radial.fillScreen(BLACK);
    _canvas_solid.fillScreen(BLACK);

    const int half_cols = GRID_COLS / 2;
    const int half_rows = GRID_ROWS / 2;
    const float d_max = (float)(half_rows * half_rows + half_cols * half_cols);

    for (int r = 0; r < GRID_ROWS; r++) {
      for (int c = 0; c < GRID_COLS; c++) {
        const float d = dist_2(r, c, half_rows, half_cols);
        const int idx =
            constrain((int)map(d, 0, d_max, 0, cell_bayer_all_count - 1), 0,
                cell_bayer_all_count - 1);
        _canvas_radial.drawBitmap(c * GRID_CELL_SIZE, r * GRID_CELL_SIZE, cell_bayer_all[idx], GRID_CELL_SIZE, GRID_CELL_SIZE, WHITE, BLACK);
        _canvas_solid.drawBitmap(c * GRID_CELL_SIZE, r * GRID_CELL_SIZE, cell_bayer_all[cell_bayer_all_count / 2], GRID_CELL_SIZE, GRID_CELL_SIZE, WHITE, BLACK);
        _canvas_random.drawBitmap(c * GRID_CELL_SIZE, r * GRID_CELL_SIZE, cell_bayer_all[random(0, cell_bayer_all_count - 1)], GRID_CELL_SIZE, GRID_CELL_SIZE, WHITE, BLACK);
      }
    }
  }

  // Blit the pre-rendered buffer into the display canvas in one memcpy.
  void draw(BackgroundType type) {
    switch (type) {
      case RADIAL:
        memcpy(display.getBuffer(), _canvas_radial.getBuffer(), BUF_SIZE);
        break;
      case SOLID:
        memcpy(display.getBuffer(), _canvas_solid.getBuffer(), BUF_SIZE);
        break;
      case RANDOM:
        memcpy(display.getBuffer(), _canvas_random.getBuffer(), BUF_SIZE);
        break;
    }
  }
};

Background background;

#endif
