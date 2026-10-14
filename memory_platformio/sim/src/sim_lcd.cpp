// Sharp Memory LCD (LS027B7DH01, 400x240) emulator. Decodes the same SPI
// byte stream the real panel receives, so display_refresh_dirty()'s dirty
// tracking, bit reversal and line addressing are exercised as on hardware.
//
// Protocol (as sent by include/display.h):
//   SS high → command byte (WRITECMD | VCOM | CLEAR bits)
//   WRITECMD: { line address (1-based), 50 data bytes, 0x00 trailer }* , 0x00
//   CLEAR:    0x00 trailer, all pixels become white
//   SS low  → end of transfer; the frame is shown

#include <Arduino.h>
#include <sim_config.h>

#include "sim.h"

#define SHARP_BIT_WRITECMD 0x01
#define SHARP_BIT_CLEAR 0x04

enum LcdState { LCD_IDLE, LCD_CMD, LCD_ADDR, LCD_DATA, LCD_LINE_TRAILER, LCD_IGNORE };

static uint8_t _fb[SIM_LCD_HEIGHT][SIM_LCD_BPL];
static LcdState _state = LCD_IDLE;
static bool _selected = false;
static bool _display_on = false;
static bool _changed = false;
static int _line = 0;
static int _col = 0;

static struct _LcdInit {
  _LcdInit() { memset(_fb, 0xFF, sizeof(_fb)); }
} _lcd_init;

void sim_lcd_spi_byte(uint8_t b) {
  if (!_selected) return;  // chip select is active-high; ignore otherwise

  switch (_state) {
    case LCD_CMD:
      if (b & SHARP_BIT_CLEAR) {
        memset(_fb, 0xFF, sizeof(_fb));
        _changed = true;
        _state = LCD_IGNORE;
      } else if (b & SHARP_BIT_WRITECMD) {
        _state = LCD_ADDR;
      } else {
        _state = LCD_IGNORE;  // VCOM-only toggle
      }
      break;
    case LCD_ADDR:
      if (b == 0 || b > SIM_LCD_HEIGHT) {
        _state = LCD_IGNORE;  // frame trailer
      } else {
        _line = b - 1;
        _col = 0;
        _state = LCD_DATA;
      }
      break;
    case LCD_DATA:
      _fb[_line][_col++] = b;
      if (_col == SIM_LCD_BPL) _state = LCD_LINE_TRAILER;
      break;
    case LCD_LINE_TRAILER:
      _changed = true;
      _state = LCD_ADDR;
      break;
    case LCD_IDLE:
    case LCD_IGNORE:
      break;
  }
}

void sim_lcd_pin(uint8_t pin, uint8_t level) {
  if (pin == SIM_PIN_DISPLAY_SS) {
    if (level && !_selected) {
      _selected = true;
      _state = LCD_CMD;
    } else if (!level && _selected) {
      _selected = false;
      _state = LCD_IDLE;
      if (_changed) {
        _changed = false;
        sim_present();
      }
    }
  } else if (pin == SIM_PIN_DISPLAY_ON) {
    if (_display_on != (level != 0)) {
      _display_on = level != 0;
      sim_present();
    }
  }
}

bool sim_lcd_display_on() { return _display_on; }

const uint8_t* sim_lcd_line(int y) { return _fb[y]; }
