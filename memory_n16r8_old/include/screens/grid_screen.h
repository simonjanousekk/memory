#ifndef GRID_SCREEN_H
#define GRID_SCREEN_H

#include <display.h>
#include <screen.h>
#include <sprites/cell.h>

const int CELL_SIZE = 16;
const int GRID_COLS = SCREEN_WIDTH / CELL_SIZE;
const int GRID_ROWS = SCREEN_HEIGHT / CELL_SIZE;

// ---------------------------------------------------------------------------
// Cell — must be defined before GridScreen uses it.
// ---------------------------------------------------------------------------
class Cell {
public:
  int x, y;
  int value;
  const uint8_t *sprite;

  Cell() : x(0), y(0), value(0) {}
  Cell(int col, int row)
      : x(col * CELL_SIZE), y(row * CELL_SIZE), value(esp_random() & 0x7),
        sprite(cell_all[value]) {}

  void draw() {
    display.drawBitmap(x, y, sprite, CELL_SIZE, CELL_SIZE, WHITE, BLACK);
  }
};

// ---------------------------------------------------------------------------
// GridScreen
// ---------------------------------------------------------------------------
class GridScreen : public Screen {
  Cell cells[GRID_COLS][GRID_ROWS];

public:
  ScreenMode id() const override { return SCREEN_GRID; }

  void generate_grid() {
    for (int x = 0; x < GRID_COLS; x++)
      for (int y = 0; y < GRID_ROWS; y++)
        cells[x][y] = Cell(x, y);
  }

  void on_button_a() override { generate_grid(); }

  void on_init() override { generate_grid(); }

  void draw() override {
    for (int x = 0; x < GRID_COLS; x++)
      for (int y = 0; y < GRID_ROWS; y++)
        cells[x][y].draw();
  }
};

#endif
