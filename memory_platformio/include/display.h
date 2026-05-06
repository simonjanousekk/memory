#ifndef DISPLAY_H
#define DISPLAY_H

#include <Adafruit_GFX.h>
#include <Arduino.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include <SPI.h>

#define SHARPMEM_BIT_WRITECMD 0x01
#define SHARPMEM_BIT_VCOM 0x02
#define SHARPMEM_BIT_CLEAR 0x04

// DISPLAY DECLARATIONS
#define SCREEN_WIDTH 400
#define SCREEN_HEIGHT 240

#define SCREEN_WIDTH_HALF 200
#define SCREEN_HEIGHT_HALF 120

#define PIN_DISPLAY_SCK 12
#define PIN_DISPLAY_MOSI 11
#define PIN_DISPLAY_SS 10
#define PIN_DISPLAY_ON 38

#define WHITE 1
#define BLACK 0

GFXcanvas1 display(SCREEN_WIDTH, SCREEN_HEIGHT);

// GFXcanvas1 stores pixels MSB-first (bit 7 = pixel x=0), but the Sharp
// display protocol expects LSB-first on the wire. Reverse each byte before
// transmission so both match.
inline uint8_t _reverse_byte(uint8_t b) {
  b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
  b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
  b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
  return b;
}

// Shadow buffer for dirty-line tracking
static uint8_t *_shadow_buffer = nullptr;
static uint8_t _display_vcom = SHARPMEM_BIT_VCOM;
static int font_height;

void display_init() {
  SPI.begin(PIN_DISPLAY_SCK, /*MISO*/ -1, PIN_DISPLAY_MOSI, /*SS*/ -1);
  pinMode(PIN_DISPLAY_SS, OUTPUT);
  digitalWrite(PIN_DISPLAY_SS, LOW);

  display.fillScreen(WHITE);

  // Send hardware clear command to the Sharp display
  SPI.beginTransaction(SPISettings(2000000, LSBFIRST, SPI_MODE0));
  digitalWrite(PIN_DISPLAY_SS, HIGH);
  SPI.write(_display_vcom | SHARPMEM_BIT_CLEAR);
  SPI.write(0x00);
  digitalWrite(PIN_DISPLAY_SS, LOW);
  SPI.endTransaction();
  _display_vcom = _display_vcom ? 0x00 : SHARPMEM_BIT_VCOM;
  // display.setFont(&FreeMonoBold9pt7b);
  // Get the font height by measuring text bounds for "A"
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds("A", 0, 0, &x1, &y1, &w, &h);
  font_height = h;
  Serial.println(font_height);

  size_t bufSize = (SCREEN_WIDTH * SCREEN_HEIGHT) / 8;
  _shadow_buffer = (uint8_t *)malloc(bufSize);
  if (_shadow_buffer)
    memset(_shadow_buffer, 0xff, bufSize); // all white

  pinMode(PIN_DISPLAY_ON, OUTPUT);
  digitalWrite(PIN_DISPLAY_ON, true);
}

// Sends only lines that changed since the last call. VCOM is toggled every
// call as required by the Sharp spec (prevents DC bias damage).
void display_refresh_dirty() {
  if (!_shadow_buffer) {
    // Fallback: send all lines without dirty tracking
    uint8_t *buf = display.getBuffer();
    const uint16_t bpl = SCREEN_WIDTH / 8;
    SPI.beginTransaction(SPISettings(8000000, LSBFIRST, SPI_MODE0));
    digitalWrite(PIN_DISPLAY_SS, HIGH);
    SPI.write(_display_vcom | SHARPMEM_BIT_WRITECMD);
    _display_vcom = _display_vcom ? 0x00 : SHARPMEM_BIT_VCOM;
    uint8_t line_buf[bpl];
    for (uint16_t line = 0; line < SCREEN_HEIGHT; line++) {
      uint8_t *src = buf + line * bpl;
      for (uint16_t b = 0; b < bpl; b++)
        line_buf[b] = _reverse_byte(src[b]);
      SPI.write(line + 1);
      SPI.writeBytes(line_buf, bpl);
      SPI.write(0x00);
    }
    SPI.write(0x00);
    digitalWrite(PIN_DISPLAY_SS, LOW);
    SPI.endTransaction();
    return;
  }

  uint8_t *buf = display.getBuffer();
  const uint16_t bpl = SCREEN_WIDTH / 8; // bytes per line

  SPI.beginTransaction(SPISettings(8000000, LSBFIRST, SPI_MODE0));
  digitalWrite(PIN_DISPLAY_SS, HIGH);

  SPI.write(_display_vcom | SHARPMEM_BIT_WRITECMD);
  _display_vcom = _display_vcom ? 0x00 : SHARPMEM_BIT_VCOM;

  uint8_t line_buf[bpl];
  for (uint16_t line = 0; line < SCREEN_HEIGHT; line++) {
    uint8_t *cur = buf + line * bpl;
    uint8_t *shd = _shadow_buffer + line * bpl;
    if (memcmp(cur, shd, bpl) == 0)
      continue;

    for (uint16_t b = 0; b < bpl; b++)
      line_buf[b] = _reverse_byte(cur[b]);
    SPI.write(line + 1);           // 1-indexed line address
    SPI.writeBytes(line_buf, bpl); // bit-reversed pixel data
    SPI.write(0x00);               // line trailer
    memcpy(shd, cur, bpl);         // shadow tracks raw canvas bytes
  }

  SPI.write(0x00); // frame trailer
  digitalWrite(PIN_DISPLAY_SS, LOW);
  SPI.endTransaction();
}

void display_power(bool power) {
  display.fillScreen(WHITE);
  display_refresh_dirty();
  digitalWrite(PIN_DISPLAY_ON, power ? HIGH : LOW);
}

// Refresh rate tracking
unsigned long _rr_frame_count = 0;
unsigned long _rr_total_micros = 0;
float _rr_avg_ms = 0;

void refresh_rate_update(unsigned long refresh_micros) {
  _rr_frame_count++;
  _rr_total_micros += refresh_micros;
  if (_rr_frame_count >= 60) {
    _rr_avg_ms = (_rr_total_micros / (float)_rr_frame_count) / 1000.0f;
    _rr_frame_count = 0;
    _rr_total_micros = 0;
  }
}

// Game update rate tracking
unsigned long _gr_tick_count = 0;
unsigned long _gr_window_start = 0;
float _gr_tps = 0;

void game_rate_update() {
  unsigned long now = micros();
  if (_gr_window_start == 0) {
    _gr_window_start = now;
    return;
  }
  _gr_tick_count++;
  if (_gr_tick_count >= 60) {
    _gr_tps = 60000000.0f / (float)(now - _gr_window_start);
    _gr_window_start = now;
    _gr_tick_count = 0;
  }
}

// (x, y) = top-left corner of the outer box (border included).
// Text is inset by `border` pixels on all sides.
void draw_text_block(String text, int x, int y, bool color = WHITE,
                     bool bg = true, int border = 2) {
  int16_t x1, y1;
  uint16_t w, h;

  display.setTextSize(1);

  // Measure at a safe y (100) to get text dimensions and ascent without
  // producing off-screen negative coordinates.
  display.getTextBounds(text, x + border, 100, &x1, &y1, &w, &h);
  int ascent = 100 - y1; // how far text top is above the cursor (baseline)

  if (bg) {
    // Place the box at (x, y), text content starts at (x+border, y+border).
    display.fillRect(x, y, w + border * 2, h + border * 2, !color);
  }

  display.setCursor(x + border, (y + border) + ascent);
  display.setTextColor(color);
  display.print(text);
}

int _debug_row_y(int row) {
  // Each debug overlay row: font_height + 2*border + 2px gap
  return (font_height + 4 + 2) * row + 1;
}

#endif