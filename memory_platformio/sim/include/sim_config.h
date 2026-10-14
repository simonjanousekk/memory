#ifndef SIM_CONFIG_H
#define SIM_CONFIG_H

// Native simulator defaults. Edit freely; most can also be overridden on the
// command line (see sim/README.md).

#include <stdint.h>

// ---------------------------------------------------------------------------
// 1-bit palette: the window color of BLACK (0) and WHITE (1) canvas pixels.
// SIM_PALETTES[0] is the default. C cycles through them at runtime, and
// --black=RRGGBB / --white=RRGGBB override the colors of the active one.
// ---------------------------------------------------------------------------
struct SimPalette {
  const char* name;
  uint32_t black;  // 0xRRGGBB
  uint32_t white;  // 0xRRGGBB
};

static const SimPalette SIM_PALETTES[] = {
    {"mono", 0x000000, 0xFFFFFF},
    {"memory lcd", 0x2A2C2E, 0xC4C8C6},
    {"inverted", 0xFFFFFF, 0x000000},
    {"paper", 0x1D1B4F, 0xF2EEE3},
};
static const int SIM_PALETTE_COUNT = sizeof(SIM_PALETTES) / sizeof(SIM_PALETTES[0]);

// Letterbox color around the display when the window isn't 5:3.
#define SIM_COLOR_FRAME 0x101010

// ---------------------------------------------------------------------------
// Window
// ---------------------------------------------------------------------------
#define SIM_DEFAULT_SCALE 2

// ---------------------------------------------------------------------------
// Pins (keep in sync with include/input.h and include/display.h)
// ---------------------------------------------------------------------------
#define SIM_PIN_BUTTON_A 6
#define SIM_PIN_BUTTON_B 7
#define SIM_PIN_ENCODER_BUTTON 8
#define SIM_PIN_POWER 15
#define SIM_PIN_DISPLAY_SS 10
#define SIM_PIN_DISPLAY_ON 38

// ---------------------------------------------------------------------------
// Encoder: counts added per arrow key press. Matches the device, where one
// detent moves one step in the UI.
// Flip SIM_ENCODER_DIRECTION if left/right feel swapped vs. the device.
// ---------------------------------------------------------------------------
#define SIM_ENCODER_COUNTS_PER_DETENT 1
#define SIM_ENCODER_DIRECTION -1  // right arrow = count decreases = on_encoder_rotate(+)

// Holding an arrow spins the encoder on its own timer (OS key repeat is
// ignored): after HOLD_DELAY the rate ramps linearly from START to MAX steps/s
// over RAMP.
#define SIM_ENCODER_HOLD_DELAY_MS 120
#define SIM_ENCODER_HOLD_START_RATE 10.0f  // steps per second
#define SIM_ENCODER_HOLD_MAX_RATE 60.0f    // steps per second
#define SIM_ENCODER_HOLD_RAMP_MS 500

// ---------------------------------------------------------------------------
// Network
// ---------------------------------------------------------------------------
#define SIM_WIFI_CONNECT_MS 1200   // time WiFi.status() takes to reach WL_CONNECTED
#define SIM_FIXTURE_LATENCY_MS 400 // fake round-trip for fixture responses
#define SIM_WIFI_RSSI (-52)

// ---------------------------------------------------------------------------
// Fuel gauge
// ---------------------------------------------------------------------------
#define SIM_BATTERY_VOLTAGE 3.95f
#define SIM_BATTERY_PERCENT 80.0f

#endif
