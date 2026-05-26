#ifndef SCREEN_H
#define SCREEN_H

// ---------------------------------------------------------------------------
// ScreenMode — add a new value here for every new screen.
// ---------------------------------------------------------------------------
enum ScreenMode {
  SCREEN_BOB = 0,
  SCREEN_ZAJAC,
  SCREEN_DEBUG_MENU,
  SCREEN_GRID,
  SCREEN_MAZE,
  SCREEN_LETTERS,
  // SCREEN_SIMONSAYS,
  SCREEN_LOGO,
  SCREEN_COUNT,
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

  // Optional — minigame screens override these so the game controller
  // can detect completion and read the final time without knowing the
  // concrete screen type. Non-minigame screens leave the defaults.
  virtual bool     is_complete() const { return false; }
  virtual uint32_t finish_ms()   const { return 0; }

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
