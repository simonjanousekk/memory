#ifndef GRID_SCREEN_H
#define GRID_SCREEN_H

#include <display.h>
#include <screen.h>

class GridScreen : public Screen {
public:
  ScreenMode id() const override { return SCREEN_GRID; }

  void draw() override {
    for (int x = 0; x < SCREEN_WIDTH; x += 20)
      display.drawFastVLine(x, 0, SCREEN_HEIGHT, BLACK);
    for (int y = 0; y < SCREEN_HEIGHT; y += 20)
      display.drawFastHLine(0, y, SCREEN_WIDTH, BLACK);
  }
};

#endif
