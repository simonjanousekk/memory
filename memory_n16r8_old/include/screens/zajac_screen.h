#ifndef ZAJAC_SCREEN_H
#define ZAJAC_SCREEN_H

#include <display.h>
#include <screen.h>
#include <sprites/zajac.h>

class ZajacScreen : public Screen {
  uint16_t _animation_index = 0;

public:
  ScreenMode id() const override { return SCREEN_ZAJAC; }
  void update() override { _animation_index++; }

  void draw() override {
    display.drawBitmap(SCREEN_WIDTH / 2 - ZAJAC_WIDTH / 2,
                       SCREEN_HEIGHT / 2 - ZAJAC_HEIGHT / 2,
                       zajac_data[_animation_index % ZAJAC_FRAME_COUNT],
                       ZAJAC_WIDTH, ZAJAC_HEIGHT, 0);
  }
};

#endif
