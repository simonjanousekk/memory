#ifndef LOGO_SCREEN_H
#define LOGO_SCREEN_H

#include <display.h>
#include <screen.h>
#include <sprites/logo_animation_sprites.h>
#include <sprites/logo_sprites.h>

class LogoScreen : public Screen {
  bool inverted = false;
  int animation_index = 0;
  uint16_t animation_time_per_frame = 1000 / 30;
  uint16_t animation_last_time = 0;

 public:
  ScreenMode
  id() const override { return SCREEN_LOGO; }
  void draw() override {
    if (inverted) {
      display.fillScreen(BLACK);
    }
    // display.drawBitmap(0, 0, logo_fullscreen_data, LOGO_WIDTH, LOGO_HEIGHT, inverted ? WHITE : BLACK, inverted ? BLACK : WHITE);
    display.drawBitmap(0, SCREEN_HEIGHT / 2 - 20, logo_animation_allArray[animation_index], 400, 40, inverted ? WHITE : BLACK, inverted ? BLACK : WHITE);
  }

  void update() override {
    if (millis() - animation_last_time >= animation_time_per_frame) {
      animation_index++;
      if (animation_index >= logo_animation_allArray_LEN) {
        animation_index = 0;
      }
      animation_last_time = millis();
    }
  }

  void on_button_a() override {
    inverted = !inverted;
  }
};

#endif