# Native simulator

Runs the firmware on macOS in an SDL window, so you can test without uploading. `src/main.cpp` and `include/` compile unchanged. The hardware is emulated at the pin and SPI level instead:

- **Display**: the SPI bytes `display_refresh_dirty()` sends are decoded with the real Sharp Memory LCD protocol, so dirty-line tracking and bit reversal run exactly as on the device. GPIO38 (DISP) low blanks the panel.
- **Buttons**: `digitalRead()` follows the keyboard, and the real OneButton library runs on top of it (long press, A+B debug menu combo).
- **`random()` after `randomSeed()`** reproduces arduino-esp32 + newlib, so a seed generates the same maze, words and count layout as on the device.
- **FreeRTOS tasks** run as `std::thread`s. **Deep sleep** blanks the window, waits for the wake pin and then restarts the process, like the ESP32's reset on wake.

## Run

Needs SDL2: `brew install sdl2`. From `memory_platformio/`:

```bash
pio run -e native && .pio/build/native/program
.pio/build/native/program --help
```

`pio run` on its own still builds only the ESP32 firmware.

## Keys

| Key | Device |
|---|---|
| D | button A |
| F | button B (hold D+F for the debug menu) |
| ← / → | encoder rotation (hold to keep turning) |
| ↑ | encoder button |
| P | power slide switch. The first press turns it off (deep sleep), the next turns it back on (wake = reset) |
| W | Wi-Fi available on/off |
| C | cycle the color palette |
| S | screenshot to `sim-<time>.bmp` |
| Esc | quit |

## Colors

The panel is 1-bit. Each palette maps `BLACK` (0) and `WHITE` (1) pixels to a window color:

- Edit or add presets in `SIM_PALETTES` in [include/sim_config.h](include/sim_config.h). The first one is the default.
- `--palette=memory\ lcd` (or `--palette=1`) picks a preset at startup. `C` cycles through them while running.
- `--black=RRGGBB --white=RRGGBB` set the colors directly, for example `--black=#3a0ca3 --white=fde68a`.

## Network

| `--net=` | Wi-Fi | Leaderboard GET | Result POST |
|---|---|---|---|
| `offline` | never connects | — | — |
| `fixture` (default) | connects after 1.2 s | `fixtures/leaderboard.json` | printed, not sent |
| `live` | connects after 1.2 s | real Supabase (libcurl) | printed, not sent |
| `live --allow-upload` | connects after 1.2 s | real Supabase | **sent to the exhibition leaderboard** |

`W` takes Wi-Fi away or brings it back while running. Once the firmware has given up on every network, it only reconnects through the debug menu's "WiFi" item, as on the device.

## Other options

`--scale=N` sets the window size (the window can also be resized). For smoke tests, `--quit-after=MS --screenshot=PATH` boots, waits, saves the panel and exits.

## Files

- `include/`: headers that replace the Arduino core, SPI, Wire, WiFi, HTTPClient, ESP32Encoder and MAX17048. Only the `native` env uses them. `sim_config.h` holds pins, encoder counts, timings and palettes.
- `src/`: the emulator itself (`sim_main.cpp` window + keys + sleep, `sim_lcd.cpp` Sharp decoder, `sim_net.cpp` Wi-Fi/HTTP, `arduino_core.cpp` time/pins/random).
- `sim_build.py`: links SDL2 and libcurl, and skips Adafruit GFX's TFT/OLED drivers.

If the firmware starts using a new Arduino/ESP API, add it to the shim headers.
