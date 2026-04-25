#ifndef DISPLAY_H
#define DISPLAY_H

#include <Adafruit_SharpMem.h>
#include <Arduino.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include <SPI.h>

// DISPLAY DECLARATIONS
#define SCREEN_WIDTH 400
#define SCREEN_HEIGHT 240

#define PIN_DISPLAY_SCK 12
#define PIN_DISPLAY_MOSI 11
#define PIN_DISPLAY_SS 10
#define PIN_DISPLAY_ON 38

#define WHITE 1
#define BLACK 0

Adafruit_SharpMem display(&SPI, PIN_DISPLAY_SS, SCREEN_WIDTH, SCREEN_HEIGHT, 2000000);

// Shadow buffer for dirty-line tracking
static uint8_t *_shadow_buffer = nullptr;
static uint8_t _display_vcom = SHARPMEM_BIT_VCOM;
static int font_height;

void display_init() {
  SPI.begin(PIN_DISPLAY_SCK, /*MISO*/ -1, PIN_DISPLAY_MOSI, /*SS*/ -1);
  display.begin();
  display.clearDisplay();
  display.setFont(&FreeMonoBold9pt7b);
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
    display.refresh();
    return;
  } // safe fallback

  uint8_t *buf = display.getBuffer();
  const uint16_t bpl = SCREEN_WIDTH / 8; // bytes per line

  SPI.beginTransaction(SPISettings(8000000, LSBFIRST, SPI_MODE0));
  digitalWrite(PIN_DISPLAY_SS, HIGH);

  SPI.write(_display_vcom | SHARPMEM_BIT_WRITECMD);
  _display_vcom = _display_vcom ? 0x00 : SHARPMEM_BIT_VCOM;

  for (uint16_t line = 0; line < SCREEN_HEIGHT; line++) {
    uint8_t *cur = buf + line * bpl;
    uint8_t *shd = _shadow_buffer + line * bpl;
    if (memcmp(cur, shd, bpl) == 0)
      continue;

    SPI.write(line + 1);      // 1-indexed line address
    SPI.writeBytes(cur, bpl); // pixel data (write-only, no buffer corruption)
    SPI.write(0x00);          // line trailer
    memcpy(shd, cur, bpl);    // update shadow
  }

  SPI.write(0x00); // frame trailer
  digitalWrite(PIN_DISPLAY_SS, LOW);
  SPI.endTransaction();
}

void display_power(bool power) {
  display.clearDisplayBuffer();
  display_refresh_dirty();
  digitalWrite(PIN_DISPLAY_ON, power ? HIGH : LOW);
}

// Refresh rate tracking
unsigned long _rr_frameCount = 0;
unsigned long _rr_totalMicros = 0;
float _rr_avgMs = 0;

void refresh_rate_update(unsigned long refreshMicros) {
  _rr_frameCount++;
  _rr_totalMicros += refreshMicros;
  if (_rr_frameCount >= 60) {
    _rr_avgMs = (_rr_totalMicros / (float)_rr_frameCount) / 1000.0f;
    _rr_frameCount = 0;
    _rr_totalMicros = 0;
  }
}

// Game update rate tracking
unsigned long _gr_tickCount = 0;
unsigned long _gr_windowStart = 0;
float _gr_tps = 0;

void game_rate_update() {
  unsigned long now = micros();
  if (_gr_windowStart == 0) {
    _gr_windowStart = now;
    return;
  }
  _gr_tickCount++;
  if (_gr_tickCount >= 60) {
    _gr_tps = 60000000.0f / (float)(now - _gr_windowStart);
    _gr_windowStart = now;
    _gr_tickCount = 0;
  }
}

// (x, y) = top-left corner of the outer box (border included).
// Text is inset by `border` pixels on all sides.
void draw_text_block(String text, int x, int y, bool color = WHITE, bool bg = true, int border = 2) {
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

void refresh_rate_debug_display() {
  char buf[40];
  snprintf(buf, sizeof(buf), "%.1fms %.0fUPS %.0fFPS",
           _rr_avgMs, 1000.0f / max(_rr_avgMs, 0.001f), _gr_tps);
  draw_text_block(buf, 1, 18, WHITE);
}

#endif