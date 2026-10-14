// Native simulator entry point: SDL window, keyboard → pins / encoder,
// 1-bit palette, deep sleep, and the Arduino setup() / loop() driver.

#define SDL_MAIN_HANDLED
#include <Arduino.h>
#include <SDL.h>
#include <curl/curl.h>
#include <limits.h>
#include <time.h>
#include <sim_config.h>
#include <unistd.h>

#include <atomic>
#include <string>
#include <thread>

#include "sim.h"

SimOptions sim_opts = {
    SIM_DEFAULT_SCALE, 0, -1, -1, SIM_NET_FIXTURE, false,
};

static std::thread::id _main_thread = std::this_thread::get_id();
static std::atomic<int64_t> _encoder_count{0};

static SDL_Window* _window = nullptr;
static SDL_Renderer* _renderer = nullptr;
static SDL_Texture* _texture = nullptr;

static const char* _screenshot_path = nullptr;  // --screenshot
static unsigned long _quit_after_ms = 0;         // --quit-after

static char _exe_path[PATH_MAX];
static char** _argv = nullptr;
static bool _asleep = false;
static int _wake_pin = -1;
static int _wake_level = HIGH;

bool sim_on_main_thread() { return std::this_thread::get_id() == _main_thread; }

int64_t sim_encoder_count() { return _encoder_count.load(); }

// ---------------------------------------------------------------------------
// Palette
// ---------------------------------------------------------------------------
static uint32_t _color_black() {
  return sim_opts.black_override >= 0 ? (uint32_t)sim_opts.black_override : SIM_PALETTES[sim_opts.palette].black;
}

static uint32_t _color_white() {
  return sim_opts.white_override >= 0 ? (uint32_t)sim_opts.white_override : SIM_PALETTES[sim_opts.palette].white;
}

static const char* _palette_name() {
  if (sim_opts.black_override >= 0 || sim_opts.white_override >= 0) return "custom";
  return SIM_PALETTES[sim_opts.palette].name;
}

static void _palette_next() {
  sim_opts.palette = (sim_opts.palette + 1) % SIM_PALETTE_COUNT;
  sim_opts.black_override = -1;
  sim_opts.white_override = -1;
  printf("[sim] palette: %s (black #%06X, white #%06X)\n", _palette_name(), _color_black(), _color_white());
  sim_update_title();
  sim_present();
}

// ---------------------------------------------------------------------------
// Rendering
// ---------------------------------------------------------------------------
// Converts the panel to ARGB8888 with the current palette.
static void _render_pixels(void* pixels, int pitch) {
  const uint32_t black = 0xFF000000 | _color_black();
  const uint32_t white = 0xFF000000 | _color_white();
  const bool on = sim_lcd_display_on();  // DISP low: panel shows all white

  for (int y = 0; y < SIM_LCD_HEIGHT; y++) {
    uint32_t* row = (uint32_t*)((uint8_t*)pixels + y * pitch);
    const uint8_t* line = sim_lcd_line(y);
    for (int x = 0; x < SIM_LCD_WIDTH; x++) {
      const bool bit = !on || ((line[x >> 3] >> (x & 7)) & 1);  // LSB-first on the wire
      row[x] = bit ? white : black;
    }
  }
}

void sim_present() {
  if (!_renderer || !sim_on_main_thread()) return;

  void* pixels;
  int pitch;
  if (SDL_LockTexture(_texture, nullptr, &pixels, &pitch) != 0) return;
  _render_pixels(pixels, pitch);
  SDL_UnlockTexture(_texture);

  SDL_SetRenderDrawColor(_renderer, (SIM_COLOR_FRAME >> 16) & 0xFF, (SIM_COLOR_FRAME >> 8) & 0xFF,
      SIM_COLOR_FRAME & 0xFF, 0xFF);
  SDL_RenderClear(_renderer);
  SDL_RenderCopy(_renderer, _texture, nullptr, nullptr);
  SDL_RenderPresent(_renderer);
}

// Saves the panel at 1x with the current palette (24-bit BMP).
static void _save_screenshot(const char* path) {
  SDL_Surface* argb = SDL_CreateRGBSurfaceWithFormat(0, SIM_LCD_WIDTH, SIM_LCD_HEIGHT, 32, SDL_PIXELFORMAT_ARGB8888);
  if (!argb) return;
  _render_pixels(argb->pixels, argb->pitch);
  SDL_Surface* rgb = SDL_ConvertSurfaceFormat(argb, SDL_PIXELFORMAT_RGB24, 0);
  if (rgb && SDL_SaveBMP(rgb, path) == 0)
    printf("[sim] screenshot: %s\n", path);
  else
    printf("[sim] screenshot failed: %s\n", SDL_GetError());
  SDL_FreeSurface(rgb);
  SDL_FreeSurface(argb);
}

void sim_update_title() {
  if (!_window) return;
  std::string title = "tot-M2 sim  |  ";
  title += _palette_name();
  title += "  |  wifi ";
  title += sim_wifi_available() ? "on" : "off";
  title += " (";
  title += sim_net_mode_name();
  title += ")";
  if (_asleep) title += "  |  asleep, P to wake";
  SDL_SetWindowTitle(_window, title.c_str());
}

// ---------------------------------------------------------------------------
// Input
// ---------------------------------------------------------------------------
static void _quit() {
  if (_screenshot_path) _save_screenshot(_screenshot_path);
  fflush(stdout);
  SDL_Quit();
  _exit(0);  // skip static destructors; detached network threads may be running
}

// Arrow keys: a tap is one detent, holding spins with acceleration.
static int _encoder_hold_dir = 0;  // -1 left, +1 right, 0 none
static uint32_t _encoder_hold_start = 0;
static uint32_t _encoder_hold_last = 0;
static float _encoder_hold_accum = 0;

static void _encoder_step(int dir, int steps) {
  _encoder_count += dir * SIM_ENCODER_DIRECTION * SIM_ENCODER_COUNTS_PER_DETENT * steps;
}

static void _encoder_key(int dir, bool down) {
  if (down) {
    _encoder_step(dir, 1);
    _encoder_hold_dir = dir;
    _encoder_hold_start = _encoder_hold_last = SDL_GetTicks();
    _encoder_hold_accum = 0;
  } else if (_encoder_hold_dir == dir) {
    _encoder_hold_dir = 0;
  }
}

static void _encoder_hold_update() {
  if (!_encoder_hold_dir) return;
  const uint32_t now = SDL_GetTicks();
  const uint32_t held = now - _encoder_hold_start;
  const float dt = (now - _encoder_hold_last) / 1000.0f;
  _encoder_hold_last = now;
  if (held < SIM_ENCODER_HOLD_DELAY_MS) return;

  const float t = fminf(1.0f, (held - SIM_ENCODER_HOLD_DELAY_MS) / (float)SIM_ENCODER_HOLD_RAMP_MS);
  const float rate = SIM_ENCODER_HOLD_START_RATE + t * (SIM_ENCODER_HOLD_MAX_RATE - SIM_ENCODER_HOLD_START_RATE);
  _encoder_hold_accum += rate * dt;
  const int steps = (int)_encoder_hold_accum;
  if (steps > 0) {
    _encoder_step(_encoder_hold_dir, steps);
    _encoder_hold_accum -= steps;
  }
}

static void _release_buttons() {
  sim_set_pin(SIM_PIN_BUTTON_A, HIGH);
  sim_set_pin(SIM_PIN_BUTTON_B, HIGH);
  sim_set_pin(SIM_PIN_ENCODER_BUTTON, HIGH);
  _encoder_hold_dir = 0;
}

// Scancodes, so the mapping follows key position on any layout.
static void _handle_key(const SDL_KeyboardEvent& e, bool down) {
  const uint8_t level = down ? LOW : HIGH;
  switch (e.keysym.scancode) {
    case SDL_SCANCODE_D:
      sim_set_pin(SIM_PIN_BUTTON_A, level);
      break;
    case SDL_SCANCODE_F:
      sim_set_pin(SIM_PIN_BUTTON_B, level);
      break;
    case SDL_SCANCODE_UP:
      sim_set_pin(SIM_PIN_ENCODER_BUTTON, level);
      break;
    case SDL_SCANCODE_LEFT:
      if (!e.repeat) _encoder_key(-1, down);
      break;
    case SDL_SCANCODE_RIGHT:
      if (!e.repeat) _encoder_key(+1, down);
      break;
    case SDL_SCANCODE_P:  // power is a slide switch: each press flips it
      if (down && !e.repeat) {
        const uint8_t next = digitalRead(SIM_PIN_POWER) == HIGH ? LOW : HIGH;
        sim_set_pin(SIM_PIN_POWER, next);
        printf("[sim] power switch %s\n", next == HIGH ? "on" : "off");
      }
      break;
    case SDL_SCANCODE_W:
      if (down && !e.repeat) {
        sim_wifi_toggle();
        sim_update_title();
      }
      break;
    case SDL_SCANCODE_C:
      if (down && !e.repeat) _palette_next();
      break;
    case SDL_SCANCODE_S:
      if (down && !e.repeat) {
        char path[64];
        snprintf(path, sizeof(path), "sim-%lu.bmp", (unsigned long)time(nullptr));
        _save_screenshot(path);
      }
      break;
    case SDL_SCANCODE_ESCAPE:
      if (down) _quit();
      break;
    default:
      break;
  }
}

void sim_pump_events() {
  if (!_window || !sim_on_main_thread()) return;
  SDL_Event e;
  while (SDL_PollEvent(&e)) {
    switch (e.type) {
      case SDL_QUIT:
        _quit();
        break;
      case SDL_KEYDOWN:
      case SDL_KEYUP:
        _handle_key(e.key, e.type == SDL_KEYDOWN);
        break;
      case SDL_WINDOWEVENT:
        if (e.window.event == SDL_WINDOWEVENT_FOCUS_LOST) _release_buttons();
        if (e.window.event == SDL_WINDOWEVENT_EXPOSED || e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
          sim_present();
        break;
    }
  }
  _encoder_hold_update();
}

// ---------------------------------------------------------------------------
// Deep sleep. On the ESP32, waking from deep sleep is a full reset, so the
// sim waits for the wake pin and then re-executes itself.
// ---------------------------------------------------------------------------
esp_err_t esp_sleep_enable_ext0_wakeup(gpio_num_t gpio, int level) {
  _wake_pin = (int)gpio;
  _wake_level = level ? HIGH : LOW;
  return ESP_OK;
}

void esp_deep_sleep_start() {
  printf("[sim] deep sleep (wake: GPIO%d == %s)\n", _wake_pin, _wake_level ? "HIGH" : "LOW");
  _asleep = true;
  sim_update_title();
  sim_present();

  while (_wake_pin < 0 || digitalRead(_wake_pin) != _wake_level) {
    sim_pump_events();
    SDL_Delay(10);
  }

  printf("[sim] wake → reset\n");
  fflush(stdout);
  SDL_Quit();
  execv(_exe_path, _argv);
  perror("[sim] execv");
  _exit(1);
}

// ---------------------------------------------------------------------------
// Command line
// ---------------------------------------------------------------------------
static void _usage(const char* exe) {
  printf(
      "usage: %s [options]\n"
      "  --scale=N           window scale (default %d)\n"
      "  --palette=NAME|N    start palette:",
      exe, SIM_DEFAULT_SCALE);
  for (int i = 0; i < SIM_PALETTE_COUNT; i++) printf(" %s%s", SIM_PALETTES[i].name, i + 1 < SIM_PALETTE_COUNT ? "," : "\n");
  printf(
      "  --black=RRGGBB      color of BLACK (0) pixels\n"
      "  --white=RRGGBB      color of WHITE (1) pixels\n"
      "  --net=MODE          offline | fixture (default) | live\n"
      "  --allow-upload      in live mode, really POST results to Supabase\n"
      "  --screenshot=PATH   save a BMP of the panel on quit\n"
      "  --quit-after=MS     quit automatically (smoke tests)\n");
}

static bool _parse_color(const char* s, int64_t* out) {
  if (*s == '#') s++;
  char* end;
  long v = strtol(s, &end, 16);
  if (end - s != 6 || *end != '\0') return false;
  *out = v;
  return true;
}

static void _parse_args(int argc, char** argv) {
  for (int i = 1; i < argc; i++) {
    const char* a = argv[i];
    bool ok = true;
    if (!strncmp(a, "--scale=", 8)) {
      sim_opts.scale = atoi(a + 8);
      ok = sim_opts.scale >= 1 && sim_opts.scale <= 8;
    } else if (!strncmp(a, "--palette=", 10)) {
      ok = false;
      for (int p = 0; p < SIM_PALETTE_COUNT; p++) {
        if (!strcmp(a + 10, SIM_PALETTES[p].name) || (isdigit(a[10]) && atoi(a + 10) == p)) {
          sim_opts.palette = p;
          ok = true;
        }
      }
    } else if (!strncmp(a, "--black=", 8)) {
      ok = _parse_color(a + 8, &sim_opts.black_override);
    } else if (!strncmp(a, "--white=", 8)) {
      ok = _parse_color(a + 8, &sim_opts.white_override);
    } else if (!strcmp(a, "--net=offline")) {
      sim_opts.net = SIM_NET_OFFLINE;
    } else if (!strcmp(a, "--net=fixture")) {
      sim_opts.net = SIM_NET_FIXTURE;
    } else if (!strcmp(a, "--net=live")) {
      sim_opts.net = SIM_NET_LIVE;
    } else if (!strcmp(a, "--allow-upload")) {
      sim_opts.allow_upload = true;
    } else if (!strncmp(a, "--screenshot=", 13)) {
      _screenshot_path = a + 13;
    } else if (!strncmp(a, "--quit-after=", 13)) {
      _quit_after_ms = strtoul(a + 13, nullptr, 10);
      ok = _quit_after_ms > 0;
    } else if (!strcmp(a, "--help") || !strcmp(a, "-h")) {
      _usage(argv[0]);
      exit(0);
    } else {
      ok = false;
    }
    if (!ok) {
      fprintf(stderr, "[sim] bad argument: %s\n", a);
      _usage(argv[0]);
      exit(2);
    }
  }
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
int main(int argc, char** argv) {
  setvbuf(stdout, nullptr, _IOLBF, 0);
  _argv = argv;
  if (!realpath(argv[0], _exe_path)) strncpy(_exe_path, argv[0], sizeof(_exe_path) - 1);

  _parse_args(argc, argv);
  sim_net_init();
  curl_global_init(CURL_GLOBAL_DEFAULT);

  SDL_SetMainReady();
  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    fprintf(stderr, "[sim] SDL_Init: %s\n", SDL_GetError());
    return 1;
  }
  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
  _window = SDL_CreateWindow("tot-M2 sim", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
      SIM_LCD_WIDTH * sim_opts.scale, SIM_LCD_HEIGHT * sim_opts.scale,
      SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
  _renderer = _window ? SDL_CreateRenderer(_window, -1, SDL_RENDERER_ACCELERATED) : nullptr;
  _texture = _renderer ? SDL_CreateTexture(_renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
                             SIM_LCD_WIDTH, SIM_LCD_HEIGHT)
                       : nullptr;
  if (!_texture) {
    fprintf(stderr, "[sim] SDL window: %s\n", SDL_GetError());
    return 1;
  }
  SDL_RenderSetLogicalSize(_renderer, SIM_LCD_WIDTH, SIM_LCD_HEIGHT);
  SDL_RenderSetIntegerScale(_renderer, SDL_TRUE);
  sim_update_title();
  sim_present();

  printf(
      "[sim] keys: D = A, F = B, Left/Right = encoder, Up = encoder button,\n"
      "[sim]       P = power switch, W = toggle wifi, C = cycle palette, S = screenshot, Esc = quit\n");

  setup();
  for (;;) {
    if (_quit_after_ms && millis() >= _quit_after_ms) _quit();
    sim_pump_events();
    loop();
    std::this_thread::sleep_for(std::chrono::microseconds(500));
  }
}
