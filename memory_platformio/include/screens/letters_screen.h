#ifndef LETTERS_SCREEN_H
#define LETTERS_SCREEN_H

#include <display.h>
#include <game_seed.h>
#include <game_state.h>
#include <screen.h>
#include <sprites/cell_sprites.h>

class LettersScreen : public Screen {
  static constexpr int GRID_CELL_SIZE = 16;
  static constexpr int GRID_COLS = SCREEN_WIDTH / GRID_CELL_SIZE;
  static constexpr int GRID_ROWS = SCREEN_HEIGHT / GRID_CELL_SIZE;
  static constexpr int HIDDEN_COUNT = cell_bayer_peak_count;

  bool used_hidden_indices[HIDDEN_COUNT] = {};
  TimedGame _timer;

  struct LetterCell {
    char letter;
    int x, y;
    int hidden_index = random(HIDDEN_COUNT);
    bool is_word = false;
    bool is_zapped = false;

    LetterCell(char letter = ' ', int x = 0, int y = 0)
        : letter(letter), x(x), y(y) {}

    void draw() {
      bool c = WHITE;
      if (is_zapped) {
        display.fillRect(x, y, GRID_CELL_SIZE, GRID_CELL_SIZE, WHITE);
        c = BLACK;
      }
      display.setTextSize(1);
      display.setTextColor(c);
      display.setCursor(x + 3, y + 16 - 3);
      display.print(letter);

      if (!is_zapped) {
        display.drawBitmap(x, y, cell_bayer_peak[hidden_index], GRID_CELL_SIZE, GRID_CELL_SIZE, BLACK);
      }
    }
  };

  LetterCell grid[GRID_COLS][GRID_ROWS];

  // Array of random words
  static constexpr int NUM_WORDS = 30;
  const char* random_words[NUM_WORDS] = {
      "BOOK", "TREE", "JUMP", "MILK", "HAPPY",
      "WATER", "APPLE", "HOUSE", "SMILE", "BREAD",
      "CHAIR", "TABLE", "CLOCK", "SHIRT", "SHOES",
      "GREEN", "PAPER", "RIVER", "SLEEP", "LUNCH",
      "WINDOW", "YELLOW", "FLOWER", "DOCTOR", "ANIMAL",
      "BANANA", "FRIEND", "SCHOOL", "STREET", "GUITAR"};

  int next_hidden_index() {
    int start = random(HIDDEN_COUNT);
    for (int i = 0; i < HIDDEN_COUNT; i++) {
      int idx = (start + i) % HIDDEN_COUNT;
      if (!used_hidden_indices[idx]) {
        used_hidden_indices[idx] = true;
        return idx;
      }
    }
    // All indices used — reset and start over
    memset(used_hidden_indices, 0, sizeof(used_hidden_indices));
    used_hidden_indices[start] = true;
    return start;
  }

  void generate_grid() {
    randomSeed(g_game_seed);
    memset(used_hidden_indices, 0, sizeof(used_hidden_indices));

    for (int i = 0; i < GRID_COLS; i++) {
      for (int j = 0; j < GRID_ROWS; j++) {
        grid[i][j] = LetterCell('A' + random(26), i * GRID_CELL_SIZE,
            j * GRID_CELL_SIZE);
      }
    }

    for (int i = 0; i < 3; i++) {
      add_word();
    }
  }

  void add_word() {
    const char* word = random_words[random(NUM_WORDS)];
    const bool vertical = random(2);
    const int len = strlen(word);
    const int hi = next_hidden_index();

    // check if the word can be added at a random position (crossing other words)
    int x, y;
    bool valid = false;
    while (!valid) {
      x = vertical ? random(GRID_COLS) : random(GRID_COLS - len);
      y = vertical ? random(GRID_ROWS - len) : random(GRID_ROWS);

      valid = true;
      for (int i = 0; i < len; i++) {
        LetterCell& cell = vertical ? grid[x][y + i] : grid[x + i][y];
        if (cell.is_word) {
          valid = false;
          break;
        }
      }
    }

    // insert the word into the grid
    for (int i = 0; i < len; i++) {
      LetterCell& cell = vertical ? grid[x][y + i] : grid[x + i][y];
      cell.letter = word[i];
      cell.hidden_index = hi;
      cell.is_word = true;
    }
  }

  void zap() {
    if (_timer.won) return;

    int newly_zapped = 0;
    for (int r = 0; r < GRID_ROWS; r++) {
      for (int c = 0; c < GRID_COLS; c++) {
        LetterCell& cell = grid[c][r];
        if (cell.is_word && !cell.is_zapped &&
            (cell.hidden_index == 0 || cell.hidden_index == 1 || cell.hidden_index == 2 || cell.hidden_index == HIDDEN_COUNT - 1 || cell.hidden_index == HIDDEN_COUNT - 2)) {
          cell.is_zapped = true;
          newly_zapped++;
        }
      }
    }

    if (newly_zapped == 0) {
      _timer.add_penalty();
    }

    check_win();
  }

  void check_win() {
    bool won = true;
    for (int r = 0; r < GRID_ROWS; r++) {
      for (int c = 0; c < GRID_COLS; c++) {
        LetterCell& cell = grid[c][r];
        if (cell.is_word && !cell.is_zapped) {
          won = false;
          break;
        }
      }
    }
    if (won) {
      _timer.complete();
    }
  }

 public:
  ScreenMode id()          const override { return SCREEN_LETTERS; }
  bool       is_complete() const override { return _timer.won; }
  uint32_t   finish_ms()   const override { return _timer.finish_ms; }

  void on_enter() override {
    generate_grid();
    _timer.begin();
  }

  void on_button_a() override { zap(); }
  void on_button_b() override { on_enter(); }

  void on_encoder_rotate(int delta) override {
    for (int r = 0; r < GRID_ROWS; r++) {
      for (int c = 0; c < GRID_COLS; c++) {
        grid[c][r].hidden_index =
            ((grid[c][r].hidden_index + delta) % HIDDEN_COUNT + HIDDEN_COUNT) %
            HIDDEN_COUNT;
      }
    }
  }

  void draw() override {
    display.fillScreen(BLACK);
    for (int r = 0; r < GRID_ROWS; r++) {
      for (int c = 0; c < GRID_COLS; c++) {
        grid[c][r].draw();
      }
    }

  }
};

#endif