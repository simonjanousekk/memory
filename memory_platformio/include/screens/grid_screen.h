#ifndef GRID_SCREEN_H
#define GRID_SCREEN_H

#include <display.h>
#include <helpers.h>
#include <screen.h>
#include <sprites/cell_sprites.h>

const int CELL_SIZE = 16;
const int GRID_COLS = SCREEN_WIDTH / CELL_SIZE;  // 400 / 16 = 25
const int GRID_ROWS = SCREEN_HEIGHT / CELL_SIZE; // 240 / 16 = 15

// ---------------------------------------------------------------------------
// SpriteSheet — pointer to any sprite array + its length.
// Add a new sheet here; GridScreen picks it up automatically.
// ---------------------------------------------------------------------------
struct SpriteSheet {
  const uint8_t *const *sprites;
  int count;
};

// ---------------------------------------------------------------------------
// Hilbert curve: map (x, y) on a 2^order square to distance d along the curve.
// order >= 1. x, y must be in [0, 2^order).
// ---------------------------------------------------------------------------
static uint32_t hilbert_xy2d(uint32_t order, uint32_t x, uint32_t y) {
  uint32_t d = 0;
  for (uint32_t s = 1u << (order - 1); s > 0; s >>= 1) {
    uint32_t rx = (x & s) ? 1u : 0u;
    uint32_t ry = (y & s) ? 1u : 0u;
    d += s * s * ((3 * rx) ^ ry);
    if (ry == 0) {
      if (rx == 1u) {
        x = (s - 1u) - x;
        y = (s - 1u) - y;
      }
      uint32_t t = x;
      x = y;
      y = t;
    }
  }
  return d;
}

/** Largest order with 2^order <= min(cols, rows). */
static int hilbert_max_order(int cols, int rows) {
  unsigned m = (unsigned)((cols < rows) ? cols : rows);
  int order = 0;
  while ((1u << (order + 1)) <= m)
    order++;
  return order ? order : 1;
}

// ---------------------------------------------------------------------------
// Cell — draws itself using whichever sheet GridScreen passes in.
// ---------------------------------------------------------------------------
class Cell {
public:
  int x, y, value;

  Cell() : x(0), y(0), value(0) {}
  Cell(int col, int row) : x(col * CELL_SIZE), y(row * CELL_SIZE), value(0) {}

  void draw(const SpriteSheet &sheet) {
    const uint8_t *sprite = sheet.sprites[value % sheet.count];
    display.drawBitmap(x, y, sprite, CELL_SIZE, CELL_SIZE, WHITE, BLACK);
  }
};

// ---------------------------------------------------------------------------
// GridScreen
// ---------------------------------------------------------------------------
class GridScreen : public Screen {
  Cell cells[GRID_COLS][GRID_ROWS];
  int active_sheet = 0;

  // Definitions after class (C++11 — no inline static).
  static const SpriteSheet sheets[];
  static const int sheet_count;

  int grid_mode = 0;
  const int grid_mode_count = 4;

public:
  ScreenMode id() const override { return SCREEN_GRID; }

  void generate_grid() {
    for (int x = 0; x < GRID_COLS; x++)
      for (int y = 0; y < GRID_ROWS; y++)
        cells[x][y] = Cell(x, y);
  }

  void randomize_grid() {
    for (int x = 0; x < GRID_COLS; x++)
      for (int y = 0; y < GRID_ROWS; y++)
        cells[x][y].value = esp_random() % sheets[active_sheet].count;
  }

  void equal_grid() {
    for (int x = 0; x < GRID_COLS; x++)
      for (int y = 0; y < GRID_ROWS; y++)
        cells[x][y].value = (x + y) % sheets[active_sheet].count;
  }

  void circ_grid() {
    for (int x = 0; x < GRID_COLS; x++) {
      for (int y = 0; y < GRID_ROWS; y++) {
        float d = dist_2(x, y, GRID_COLS / 2, GRID_ROWS / 2);
        cells[x][y].value = (int)map(
            d, 0, GRID_COLS / 2 * GRID_COLS / 2 + GRID_ROWS / 2 * GRID_ROWS / 2,
            0, sheets[active_sheet].count);
      }
    }
  }

  void next_grid_mode() { grid_mode = (grid_mode + 1) % grid_mode_count; }

  void update_grid() {
    switch (grid_mode) {
    case 0:
      randomize_grid();
      break;
    case 1:
      hilbert_curve_grid();
      break;
    case 2:
      equal_grid();
      break;
    case 3:
      circ_grid();
      break;
    }
  }

  void hilbert_curve_grid() {
    const int order = hilbert_max_order(GRID_COLS, GRID_ROWS);
    const uint32_t size = 1u << (unsigned)order;
    const uint32_t cells_per_tile = size * size;
    const int tiles_x = (GRID_COLS + (int)size - 1) / (int)size;
    int count = sheets[active_sheet].count;
    if (count < 1)
      count = 1;
    for (int x = 0; x < GRID_COLS; x++) {
      for (int y = 0; y < GRID_ROWS; y++) {
        const uint32_t lx = (uint32_t)(x % (int)size);
        const uint32_t ly = (uint32_t)(y % (int)size);
        const uint32_t tx = (uint32_t)(x / (int)size);
        const uint32_t ty = (uint32_t)(y / (int)size);
        const uint32_t d = hilbert_xy2d((uint32_t)order, lx, ly);
        const uint64_t idx =
            (uint64_t)d +
            (uint64_t)(tx + ty * (uint32_t)tiles_x) * cells_per_tile;
        cells[x][y].value = (int)(idx % (uint32_t)count);
      }
    }
  }

  void on_init() override {
    generate_grid();
    update_grid();
  }

  void on_button_a() override {
    next_grid_mode();
    update_grid();
  }

  void on_button_b() override {
    active_sheet = (active_sheet + 1) % sheet_count;
    update_grid();
  }

  void on_encoder_rotate(int delta) override {
    int count = sheets[active_sheet].count;
    for (int x = 0; x < GRID_COLS; x++)
      for (int y = 0; y < GRID_ROWS; y++)
        cells[x][y].value = (cells[x][y].value + delta + count * 128) % count;
  }

  void draw() override {
    const SpriteSheet &sheet = sheets[active_sheet];
    for (int x = 0; x < GRID_COLS; x++)
      for (int y = 0; y < GRID_ROWS; y++)
        cells[x][y].draw(sheet);
  }
};

// C++11 static member definitions (add new sheets here too).
const SpriteSheet GridScreen::sheets[] = {
    {cell_circle_all, cell_circle_all_count},
    {cell_bayer_all, cell_bayer_count},
};
const int GridScreen::sheet_count =
    sizeof(GridScreen::sheets) / sizeof(GridScreen::sheets[0]);

#endif
