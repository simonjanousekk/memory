// #ifndef SIMONS_SAYS_SCREEN_H
// #define SIMONS_SAYS_SCREEN_H

// #include <Arduino.h>
// #include <background.h>
// #include <display.h>
// #include <game_seed.h>
// #include <screen.h>

// enum ShapeType {
//   SHAPE_CIRCLE,
//   SHAPE_SQUARE,
//   SHAPE_TRIANGLE,
//   SHAPE_DIAMOND,
//   SHAPE_PLUS,
// };

// ShapeType random_shape() {
//   return static_cast<ShapeType>(random(0, 5));
// }

// ShapeType next_shape(ShapeType type) {
//   return static_cast<ShapeType>((type + 1) % 5);
// }

// class SimonSaysScreen : public Screen {
//   static constexpr unsigned long SHOW_DURATION = 600;
//   static constexpr unsigned long INTER_PAUSE = 200;
//   static constexpr unsigned long END_PAUSE = 1200;

//   bool _won = false;

//   enum AnimPhase {
//     PHASE_SHOW,
//     PHASE_INTER_PAUSE,
//     PHASE_END_PAUSE
//   };

//   static constexpr int sequence_length = 3;
//   ShapeType sequence[sequence_length];
//   AnimPhase phase = PHASE_END_PAUSE;
//   unsigned int shape_animation_index = 0;
//   unsigned long phase_start_time = 0;

//   void generate_sequence() {
//     for (int i = 0; i < sequence_length; i++) {
//       sequence[i] = random_shape();
//     }
//   }

//   struct Shape {
//     int size = 32;
//     ShapeType type;

//     Shape(ShapeType type) {
//       this->type = type;
//     }
//     Shape() : Shape(random_shape()) {}
//     Shape(int size) : Shape(random_shape()) {
//       this->size = size;
//     }

//     void draw(int x, int y) {
//       switch (type) {
//         case SHAPE_CIRCLE:
//           display.fillCircle(x, y, size / 2, WHITE);
//           break;
//         case SHAPE_SQUARE:
//           display.fillRect(x - size / 2, y - size / 2, size, size, WHITE);
//           break;
//         case SHAPE_TRIANGLE:
//           display.fillTriangle(x, y - size * 3 / 8, x + size * 7 / 16, y + size * 3 / 8, x - size * 7 / 16, y + size * 3 / 8, WHITE);
//           break;
//         case SHAPE_DIAMOND:
//           display.fillTriangle(x, y - size / 2, x + size / 2, y, x - size / 2, y, WHITE);
//           display.fillTriangle(x, y + size / 2, x + size / 2, y, x - size / 2, y, WHITE);
//           break;
//         case SHAPE_PLUS:
//           display.fillRect(x - size / 6, y - size / 2, size / 3, size, WHITE);
//           display.fillRect(x - size / 2, y - size / 6, size, size / 3, WHITE);
//           break;
//       }
//     }
//   };

//   Shape main_shape = Shape(64);
//   Shape selection_shapes[sequence_length];
//   int selection_index = 0;

//  public:
//   ScreenMode
//   id() const override { return SCREEN_SIMONSAYS; }

//   void on_enter() override {
//     generate_sequence();
//     shape_animation_index = 0;
//     selection_index = 0;
//     phase = PHASE_END_PAUSE;
//     phase_start_time = millis();

//     for (int i = 0; i < sequence_length; i++) {
//       selection_shapes[i] = Shape();
//     }
//   }

//   void on_button_a() override {
//     selection_shapes[selection_index].type = next_shape(selection_shapes[selection_index].type);
//   }

//   void on_button_b() override {
//     submit_selection();
//   }

//   void on_encoder_rotate(int delta) override {
//     selection_index += delta;
//     if (selection_index >= sequence_length) {
//       selection_index = 0;
//     }
//     if (selection_index < 0) {
//       selection_index = sequence_length - 1;
//     }
//   }

//   void submit_selection() {
//     for (int i = 0; i < sequence_length; i++) {
//       if (selection_shapes[i].type != sequence[i]) {
//         Serial.print("Expected: ");
//         Serial.println(sequence[i]);
//         Serial.print("Selected: ");
//         Serial.println(selection_shapes[i].type);
//         Serial.print("Index: ");
//         Serial.println(i);
//         Serial.println("Wrong selection");
//         return;
//       }
//     }
//     Serial.println("Correct selection");
//     _won = true;
//   }

//   void update() override {
//     unsigned long elapsed = millis() - phase_start_time;
//     switch (phase) {
//       case PHASE_SHOW:
//         if (elapsed >= SHOW_DURATION) {
//           phase_start_time = millis();
//           bool last = (shape_animation_index == sequence_length - 1);
//           phase = last ? PHASE_END_PAUSE : PHASE_INTER_PAUSE;
//         }
//         break;
//       case PHASE_INTER_PAUSE:
//         if (elapsed >= INTER_PAUSE) {
//           shape_animation_index++;
//           phase = PHASE_SHOW;
//           phase_start_time = millis();
//         }
//         break;
//       case PHASE_END_PAUSE:
//         if (elapsed >= END_PAUSE) {
//           shape_animation_index = 0;
//           phase = PHASE_SHOW;
//           phase_start_time = millis();
//         }
//         break;
//     }
//   }

//   void draw() override {
//     background.draw(RADIAL);

//     int x = SCREEN_WIDTH / 2;
//     int y = SCREEN_HEIGHT / 2 - main_shape.size;

//     int pad = 14;
//     int width = 4;
//     display.fillRect(x - main_shape.size / 2 - pad, y - main_shape.size / 2 - pad, main_shape.size + pad * 2, main_shape.size + pad * 2, WHITE);
//     display.fillRect(x - main_shape.size / 2 - pad + width, y - main_shape.size / 2 - pad + width, main_shape.size + pad * 2 - width * 2, main_shape.size + pad * 2 - width * 2, BLACK);

//     if (phase == PHASE_SHOW) {
//       main_shape.type = sequence[shape_animation_index];
//       main_shape.draw(x, y);
//     }

//     int space = 32;
//     for (int i = 0; i < sequence_length; i++) {
//       int x = SCREEN_WIDTH / 2 - (selection_shapes[i].size + space) * (1 - i);
//       int y = SCREEN_HEIGHT / 2 + selection_shapes[i].size * 2;
//       if (i == selection_index) {
//         display.fillRect(x - selection_shapes[i].size / 2 - pad, y - selection_shapes[i].size / 2 - pad, selection_shapes[i].size + pad * 2, selection_shapes[i].size + pad * 2, WHITE);
//       }
//       display.fillRect(x - selection_shapes[i].size / 2 - pad + width, y - selection_shapes[i].size / 2 - pad + width, selection_shapes[i].size + pad * 2 - width * 2, selection_shapes[i].size + pad * 2 - width * 2, BLACK);
//       selection_shapes[i].draw(x, y);
//     }

//     if (_won) {
//       draw_text_block("YOU WON!", SCREEN_WIDTH / 2 - 20, SCREEN_HEIGHT / 2 - 8);
//     }
//   }
// };

// #endif