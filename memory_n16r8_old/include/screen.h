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
};

// ---------------------------------------------------------------------------
// Screen — base class for all screens, Godot-style.
// Subclass it, override what you need, register an instance in main.cpp.
// ---------------------------------------------------------------------------
class Screen {
public:
  virtual ~Screen() {}
  virtual ScreenMode id() const = 0;   // identity — avoids a redundant global
  virtual void on_enter() {}           // called once when screen becomes active
  virtual void on_exit() {}            // called once when screen is left
  virtual void update() {}             // called on every game tick
  virtual void draw() = 0;            // called every render frame
};

extern Screen *currentScreen;
extern void set_screen(ScreenMode mode);

// Saved before entering debug menu so we can return to the right screen.
Screen *previousScreen = nullptr;

void toggle_debug_menu() {
  if (currentScreen->id() == SCREEN_DEBUG_MENU) {
    if (previousScreen) {
      currentScreen->on_exit();
      currentScreen = previousScreen;
      currentScreen->on_enter();
    } else {
      set_screen(SCREEN_BOB); // fallback if entered without toggle
    }
  } else {
    previousScreen = currentScreen;
    set_screen(SCREEN_DEBUG_MENU);
  }
}

void cycle_screen() {
  static const ScreenMode cycle[] = { SCREEN_BOB, SCREEN_ZAJAC, SCREEN_GRID };
  static const int n = sizeof(cycle) / sizeof(cycle[0]);
  ScreenMode cur = currentScreen->id();
  for (int i = 0; i < n; i++) {
    if (cycle[i] == cur) { set_screen(cycle[(i + 1) % n]); return; }
  }
  set_screen(cycle[0]); // fallback from debug menu or unknown
}

#endif
