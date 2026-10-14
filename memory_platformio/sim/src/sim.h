#ifndef SIM_INTERNAL_H
#define SIM_INTERNAL_H

// Declarations shared between the simulator's own translation units.
// Firmware code never includes this; it only sees the shim headers.

#include <stdint.h>

#define SIM_LCD_WIDTH 400
#define SIM_LCD_HEIGHT 240
#define SIM_LCD_BPL (SIM_LCD_WIDTH / 8)

enum SimNetMode { SIM_NET_OFFLINE, SIM_NET_FIXTURE, SIM_NET_LIVE };

struct SimOptions {
  int scale;
  int palette;          // index into SIM_PALETTES
  int64_t black_override;  // 0xRRGGBB or -1
  int64_t white_override;  // 0xRRGGBB or -1
  SimNetMode net;
  bool allow_upload;
};

extern SimOptions sim_opts;

// sim_main.cpp: window, events, title
bool sim_on_main_thread();
void sim_pump_events();
void sim_present();
void sim_update_title();

// arduino_core.cpp: pin levels
void sim_set_pin(uint8_t pin, uint8_t level);

// sim_lcd.cpp: Sharp Memory LCD emulation
void sim_lcd_pin(uint8_t pin, uint8_t level);
bool sim_lcd_display_on();
// Raw line data as it arrived on the wire: LSB-first, bit set = white.
const uint8_t* sim_lcd_line(int y);

// sim_net.cpp: WiFi availability (W key)
void sim_net_init();
bool sim_wifi_available();
void sim_wifi_toggle();
const char* sim_net_mode_name();

#endif
