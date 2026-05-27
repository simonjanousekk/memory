#ifndef SCREEN_H
#define SCREEN_H

// ---------------------------------------------------------------------------
// ScreenMode — add a new value here for every new screen.
// ---------------------------------------------------------------------------
enum ScreenMode {
  // Dev / utility screens
  SCREEN_BOB = 0,
  SCREEN_ZAJAC,
  SCREEN_DEBUG_MENU,
  SCREEN_GRID,
  // Minigames
  SCREEN_MAZE,
  SCREEN_LETTERS,
  SCREEN_COUNT,
  // Game flow screens
  SCREEN_LOGO,
  SCREEN_INTRO,
  SCREEN_OPPONENT,
  SCREEN_RESULT,
  SCREEN_GO_AGAIN,
  SCREEN_NAME_ENTRY,
  SCREEN_UPLOAD,
  SCREEN_GOODBYE,
  SCREEN_TEST,
};

// ---------------------------------------------------------------------------
// Screen — base class for all screens, Godot-style.
// Subclass it, override what you need, register an instance in main.cpp.
// ---------------------------------------------------------------------------
class Screen {
  bool _initialized = false;

 public:
  virtual ~Screen() {}
  virtual ScreenMode id() const = 0;
  virtual void on_init() {}   // called once ever, before first on_enter
  virtual void on_enter() {}  // called every time screen becomes active
  virtual void on_exit() {}   // called every time screen is left
  virtual void update() {}    // called on every game tick
  virtual void draw() = 0;    // called every render frame

  virtual void on_button_a() {}
  virtual void on_button_b() {}
  virtual void on_encoder_press() {}
  virtual void on_encoder_rotate(int delta) {}

  // Optional — screens override these for game-controller integration.
  // Non-interactive screens leave the defaults (false / 0 / 0).
  virtual bool is_complete() const { return false; }
  virtual uint32_t finish_ms() const { return 0; }
  // For branching screens (e.g. GoAgain): return 0 = primary, 1 = alternate.
  virtual int complete_result() const { return 0; }

  void ensure_init() {
    if (!_initialized) {
      _initialized = true;
      on_init();
    }
  }
};

extern Screen* currentScreen;
extern void set_screen(ScreenMode mode);

// Saved before entering debug menu so we can return to the right screen.
Screen* previousScreen = nullptr;

void toggle_debug_menu() {
  if (currentScreen->id() == SCREEN_DEBUG_MENU) {
    if (previousScreen) {
      currentScreen->on_exit();
      currentScreen = previousScreen;
      currentScreen->on_enter();
    } else {
      set_screen(SCREEN_BOB);  // fallback if entered without toggle
    }
  } else {
    previousScreen = currentScreen;
    set_screen(SCREEN_DEBUG_MENU);
  }
}

#endif
