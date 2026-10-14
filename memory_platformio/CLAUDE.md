# Firmware: memory_platformio

Target: ESP32-S3-WROOM-1 N16R8 (16 MB flash, 8 MB OPI PSRAM). Single env `esp32-s3-wroom-1-n16r8`, board `esp32-s3-devkitm-1`, Arduino framework.

## Serial / USB

The PCB has **no USB-to-serial chip**. The USB-C D+/D− lines go straight to the ESP32-S3's own USB pins (IO19/IO20). Two build flags make `Serial` work over that connection:
- `ARDUINO_USB_MODE=1` uses the chip's built-in "USB Serial/JTAG" hardware (not TinyUSB).
- `ARDUINO_USB_CDC_ON_BOOT=1` maps `Serial` to that USB port, so `Serial.print` and `pio device monitor` work over the same cable used for flashing.

What this means in practice:
- The serial port (`/dev/cu.usbmodem*`) is created by the running chip, so it **disappears** whenever the chip isn't running: during reset, after a crash, and in deep sleep (power button P). The monitor reconnects once the chip boots again.
- Output printed in the first moments after boot can be lost before the port connects. That's why `setup()` has `delay(100)` after `Serial.begin`.
- If uploading fails because the port is missing (for example, the firmware crashes straight after boot), put the chip into download mode manually: hold BOOT, tap RESET, release BOOT. The two `TS-1088` buttons on the PCB are likely BOOT and RESET.
- The baud rate (`monitor_speed = 115200`) doesn't matter over native USB. It's kept only for convention.

## Build

`pio` is not on PATH in non-interactive shells. Use `~/.platformio/penv/bin/pio` (written as `pio` below).

```bash
pio run                 # build. Run this after every change to verify
pio run -t upload       # flash. Ask first, needs the device plugged in
pio device monitor      # 115200 baud
pio run -e native && .pio/build/native/program   # native simulator, see below
```

`default_envs` is the ESP32 env, so a bare `pio run` never builds the simulator.

## Native simulator (`sim/`)

`env:native` compiles the same `src/main.cpp` for macOS against shim headers in `sim/include/` (Arduino core, SPI, Wire, WiFi, HTTPClient, ESP32Encoder, MAX17048). The firmware has no `#ifdef SIM`, so keep it that way. Hardware is emulated at the pin/SPI level: the Sharp LCD protocol is decoded from SPI bytes, and buttons drive `digitalRead()` under the real OneButton. Keys: D/F = A/B, ←/→ = encoder, ↑ = encoder button, P = power switch, W = Wi-Fi, C = palette. Network defaults to local fixtures; `--net=live` reads Supabase, and uploads need `--allow-upload` (ask first, it writes to the live leaderboard). Details in [sim/README.md](sim/README.md).

- If firmware starts using a new Arduino/ESP API, add it to the shims, or the native build breaks.
- Verify UI changes without hardware: `.pio/build/native/program --quit-after=3000 --screenshot=/tmp/x.bmp` and look at the BMP.
- `long` is 64-bit on macOS but 32-bit on the ESP32. Keep that in mind for overflow-dependent code.

## Architecture (header-only)

The code is almost entirely **header-only**. `src/main.cpp` is the only translation unit, and every `include/*.h` defines globals and functions directly (`static`/non-inline definitions are common). Because of that:
- Never include a header from a second `.cpp`, or you get duplicate symbols. New code goes in a new header that `main.cpp` includes (directly or transitively).
- Include order in `main.cpp` matters, because headers depend on globals from earlier headers.
- Includes use angle brackets relative to `include/` (`#include <screens/maze/maze_screen.h>`).
- `include/OLD/` is dead code that nothing includes.

**Main loop** (`main.cpp`): input + Wi-Fi polling every iteration. A 30 Hz game tick calls `currentScreen->update()` and then `game_controller.update()`. A separate 30 Hz render does `display.fillScreen(WHITE)` → `currentScreen->draw()` → debug overlays → `display_refresh_dirty()`.

**Display** (`display.h`): `display` is an `ExtGFXcanvas1` (an Adafruit GFX 1-bit canvas plus `drawLineThick`). `WHITE = 1`, `BLACK = 0`, and the background is white. `display_refresh_dirty()` diffs against a shadow buffer and sends only the changed lines to the Sharp LCD (bytes are bit-reversed for LSB-first SPI). VCOM toggles on every refresh, so keep calling it every frame.

**Screens** (`screen.h`): Godot-style base class with `on_init` (once), `on_enter`, `on_exit`, `update`, `draw`, and input hooks (`on_button_a/b`, `on_encoder_press`, `on_encoder_rotate`). Game-flow screens also report `is_complete()`, `finish_ms()` and `complete_result()`.

Adding a screen:
1. Add a value to `enum ScreenMode` in `screen.h`.
2. Create `include/screens/<name>_screen.h` with a subclass that implements `id()` and `draw()`.
3. In `main.cpp`: include it, add a global instance, add a `case` to `set_screen()`, and optionally add a `DebugMenuItem`.
4. If it's part of the game flow, add a `GamePhase` and map it in `_phase_screen()` in `game_controller.h`.

**Game flow** (`game_controller.h`): a phase state machine: LOGO → INTRO → (OPPONENT) → MAZE → LETTERS → COUNT → RESULT → GO_AGAIN → NAME_ENTRY. The first run is a solo tutorial. Minigames are deterministic from `g_game_seed`, which the controller sets from `current_round.*_seed` before each minigame, so you can replay against an opponent's recorded time. Session and leaderboard structs live in `game_state.h`.

**Input** (`input.h`): OneButton for A (GPIO6), B (GPIO7), power P (GPIO15, triggers deep sleep) and the encoder button (GPIO8). ESP32Encoder half-quad on GPIO17/18. Holding A+B toggles the debug menu. `screen_input_allowed()` ignores input for 800 ms after a screen change, except in the debug menu and minigames.

**Network**: `wifi/wifi_manager.h` cycles through `WIFI_NETWORKS` from the gitignored `wifi_credentials.h`. `supabase.h` fetches the leaderboard and calls the `insert-player` edge function over HTTPS with ArduinoJson.

## Assets

- `include/sprites/*.h` come from `sprite_convertor/script.py` (1-bit, MSB-first, one `uint8_t` array per frame plus `<NAME>_WIDTH/HEIGHT/FRAME_COUNT` macros). Regenerate them instead of hand-editing, and don't read them in full.
- `include/fonts/*.h` are Adafruit GFX `GFXfont` data (Panell typeface). Generated, so treat them like sprites.

## Style

`.clang-format`: Google style with no column limit. The code uses snake_case functions and variables, PascalCase classes, `_leading_underscore` for file-private globals and members, `UPPER_CASE` for macros and constants, and `// ----` banner comments to separate sections.
