#ifndef BOB_SCREEN_H
#define BOB_SCREEN_H

#include <display.h>
#include <screen.h>
#include <sprites/bob.h>

class BobScreen : public Screen {
  uint16_t _animation_index = 0;

public:
  ScreenMode id() const override { return SCREEN_BOB; }
  void update() override { _animation_index++; }

  void draw() override {
    display.drawBitmap(0, 0, bob_data[_animation_index % BOB_FRAME_COUNT],
                       BOB_WIDTH, BOB_HEIGHT, 1, 0);
  }
};

#endif
