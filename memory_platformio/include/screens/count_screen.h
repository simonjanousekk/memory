#ifndef COUNT_SCREEN_H
#define COUNT_SCREEN_H

#include <Arduino.h>
#include <background.h>
#include <display.h>
#include <game_seed.h>
#include <game_state.h>
#include <input.h>
#include <screen.h>
#include <sprites/number_sprites.h>

enum ShapeType {
  SHAPE_CIRCLE,
  SHAPE_SQUARE,
  SHAPE_TRIANGLE,
  SHAPE_DIAMOND,
  SHAPE_PLUS,
};

ShapeType random_shape() {
  return static_cast<ShapeType>(random(0, 5));
}

ShapeType next_shape(ShapeType type) {
  return static_cast<ShapeType>((type + 1) % 5);
}

const int field_width = 360;
const int field_height = 160;
const int field_x = (SCREEN_WIDTH - field_width) / 2;
// const int field_y = (SCREEN_HEIGHT - field_height) / 2;
const int field_y = 20;

class CountScreen : public Screen {
  struct Shape {
    ShapeType type = random_shape();
    int size = 20;
    int full_shape_pad = 2;
    float x, y;
    int speed = random(1, 4);
    float direction = random(TWO_PI);

    Shape() : x(random(field_x + size / 2, field_x + field_width - size / 2)), y(random(field_y + size / 2, field_y + field_height - size / 2)) {}
    Shape(ShapeType type) : Shape() { this->type = type; }
    Shape(ShapeType type, int x, int y, int size) : Shape() {
      this->type = type;
      this->x = x;
      this->y = y;
      this->size = size;
    }

    void update() {
      x += speed * cos(direction);
      y += speed * sin(direction);

      if (x < field_x + size / 2 || x > field_x + field_width - size / 2 || y < field_y + size / 2 || y > field_y + field_height - size / 2) {
        direction += PI;
      }
    }

    void collide(Shape& other) {
      float dx = other.x - x;
      float dy = other.y - y;
      float min_dist = (size + other.size) / 2.0f;
      if (dx * dx + dy * dy < min_dist * min_dist) {
        direction += PI;
        other.direction += PI;
      }
    }

    void draw() {
      switch (type) {
        case SHAPE_CIRCLE:
          display.fillCircle(x, y, size / 2 - full_shape_pad, BLACK);
          break;
        case SHAPE_SQUARE:
          display.fillRect(x - size / 2 + full_shape_pad, y - size / 2 + full_shape_pad, size - full_shape_pad * 2, size - full_shape_pad * 2, BLACK);
          break;
        case SHAPE_TRIANGLE:
          display.fillTriangle(x, y - size * 3 / 8, x + size * 7 / 16, y + size * 3 / 8, x - size * 7 / 16, y + size * 3 / 8, BLACK);
          break;
        case SHAPE_DIAMOND:
          display.fillTriangle(x, y - size / 2, x + size / 2, y, x - size / 2, y, BLACK);
          display.fillTriangle(x, y + size / 2, x + size / 2, y, x - size / 2, y, BLACK);
          break;
        case SHAPE_PLUS:
          display.fillRect(x - size / 6, y - size / 2, size / 3, size, BLACK);
          display.fillRect(x - size / 2, y - size / 6, size, size / 3, BLACK);
          break;
      }
    }

    void draw_at(int x, int y, int size) {
      this->x = x;
      this->y = y;
      this->size = size;
      draw();
    }
  };

  static constexpr int max_bar_height = 48;
  static constexpr int min_bar_height = 16;

  struct Bar {
    int y;
    int height = random(min_bar_height, max_bar_height);
    bool direction = random(0, 2) == 0;
    bool visible = true;

    Bar() : y(0) {}
    Bar(int y) : y(y) {}

    void update() {
      y += direction ? 1 : -1;

      height += random(-4, 4);
      if (height > max_bar_height) {
        height = max_bar_height;
      }
      if (height < min_bar_height) {
        height = min_bar_height;
      }

      if (visible) {
        if (random(100) < 30) {
          visible = false;
        }
      } else {
        if (random(100) < 70) {
          visible = true;
        }
      }

      if (y < field_y || y > field_y + field_height - height) {
        direction = !direction;
      }
    }

    void draw() {
      if (visible) {
        if (y + height > field_y + field_height) {
          display.fillRect(field_x, y, field_width, field_y + field_height - y, BLACK);
        } else {
          display.fillRect(field_x, y, field_width, height, BLACK);
        }
      }
    }
  };

  const int bottom_center_y = SCREEN_HEIGHT - (SCREEN_HEIGHT - (field_height + field_x)) / 2;
  const int bottom_center_x = SCREEN_WIDTH / 2;

  ShapeType main_shape_type;
  Shape main_shape;
  int main_shape_count = 0;
  int all_shapes_count = 0;
  Shape all_shapes[20];

  static constexpr int all_bars_count = 5;
  Bar all_bars[all_bars_count];

  int selected_count = 1;
  TimedGame _timer;

 public:
  ScreenMode id()          const override { return SCREEN_COUNT; }
  bool       is_complete() const override { return _timer.won; }
  uint32_t   finish_ms()   const override { return _timer.finish_ms; }

  void on_enter() override {
    randomSeed(g_game_seed);

    main_shape_type = random_shape();
    main_shape = Shape(main_shape_type);
    main_shape_count = random(1, 8);
    all_shapes_count = random(1, 8) + main_shape_count;

    for (int i = 0; i < main_shape_count; i++) {
      all_shapes[i] = Shape(main_shape_type);
    }
    for (int i = main_shape_count; i < all_shapes_count; i++) {
      all_shapes[i] = Shape();
    }

    int step = field_height / all_bars_count;
    for (int i = 0; i < all_bars_count; i++) {
      all_bars[i] = Bar(i * step + field_y);
    }

    selected_count = 1;
    _timer.begin();
  }

  void on_button_a() override {
    if (_timer.won) return;
    if (selected_count == main_shape_count) {
      _timer.complete();
    } else {
      _timer.add_penalty();
    }
  }

  void on_button_b() override { on_enter(); }

  void on_encoder_rotate(int delta) override {
    selected_count += delta;
    if (selected_count < 1) {
      selected_count = 1;
    } else if (selected_count > 8) {
      selected_count = 8;
    }
  }

  void update() override {
    if (button_a.debouncedValue() == 1) {
      for (int i = 0; i < all_shapes_count; i++) {
        all_shapes[i].update();
      }
      for (int i = 0; i < all_bars_count; i++) {
        all_bars[i].update();
      }

      // for (int i = 0; i < all_shapes_count; i++) {
      //   for (int j = i + 1; j < all_shapes_count; j++) {
      //     all_shapes[i].collide(all_shapes[j]);
      //   }
      // }
    }
  }

  void draw() override {
    background.draw(SOLID);
    int pad = 4;
    display.fillRect(field_x - pad, field_y - pad, field_width + pad * 2, field_height + pad * 2, BLACK);
    display.fillRect(field_x, field_y, field_width, field_height, WHITE);

    for (int i = 0; i < all_shapes_count; i++) {
      all_shapes[i].draw();
    }
    for (int i = 0; i < all_bars_count; i++) {
      all_bars[i].draw();
    }

    display.fillRect(bottom_center_x - 52, bottom_center_y - 20, 52 * 2, 40, WHITE);

    display.fillCircle(bottom_center_x, bottom_center_y, 2, BLACK);
    display.drawBitmap(bottom_center_x + 16, bottom_center_y - 16, number_sprites[selected_count], 32, 32, BLACK);
    main_shape.draw_at(bottom_center_x - 32, bottom_center_y, 32);

  }
};

#endif