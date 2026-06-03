#ifndef DISPLAY_H
#define DISPLAY_H

#include <Adafruit_GFX.h>
#include <Arduino.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include <SPI.h>
#include <fonts/Panell_Extended24.h>
#include <fonts/Panell_Regular12.h>
#include <fonts/Panell_Regular9.h>

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

// GFXcanvas1 extended with thick-line drawing.
class ExtGFXcanvas1 : public GFXcanvas1 {
 public:
  ExtGFXcanvas1(uint16_t w, uint16_t h) : GFXcanvas1(w, h) {}

  // Draws a thick line from (x0,y0) to (x1,y1) with true constant width at
  // every angle. For each scanline in the bounding box it finds the horizontal
  // run of pixels whose distance to the nearest point on the segment is ≤
  // weight/2, then fills it with drawFastHLine. The shape is a "stadium"
  // (rectangle + two semicircular caps), so round caps are free.
  // Pass caps=false to skip the semicircular ends (e.g. for polyline joints).
  void drawLineThick(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
      uint8_t weight, uint16_t color, bool caps = true) {
    if (weight == 0) return;
    if (weight == 1) {
      drawLine(x0, y0, x1, y1, color);
      return;
    }

    const float r = weight * 0.5f;
    const float r2 = r * r;
    const float dx = x1 - x0, dy = y1 - y0;
    const float len2 = dx * dx + dy * dy;
    const int16_t pad = (int16_t)ceilf(r);

    const int16_t xmin = max((int16_t)0, (int16_t)(min(x0, x1) - pad));
    const int16_t xmax = min((int16_t)(width() - 1), (int16_t)(max(x0, x1) + pad));
    const int16_t ymin = max((int16_t)0, (int16_t)(min(y0, y1) - pad));
    const int16_t ymax = min((int16_t)(height() - 1), (int16_t)(max(y0, y1) + pad));

    if (len2 == 0.0f) {
      if (caps) fillCircle(x0, y0, pad, color);
      return;
    }

    const float inv_len2 = 1.0f / len2;

    for (int16_t y = ymin; y <= ymax; y++) {
      const float py = (float)(y - y0);
      const float t_y = py * dy * inv_len2;  // y contribution to projection

      int16_t seg_start = -1;
      bool was_inside = false;

      for (int16_t x = xmin; x <= xmax; x++) {
        const float px = (float)(x - x0);
        const float t = px * dx * inv_len2 + t_y;
        const float tc = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
        const float cx = px - tc * dx;
        const float cy = py - tc * dy;

        // With caps=false, require projection to land on the segment (flat ends).
        const bool inside = (cx * cx + cy * cy <= r2) &&
                            (caps || (t >= 0.0f && t <= 1.0f));

        if (inside && !was_inside) {
          seg_start = x;
        } else if (!inside && was_inside) {
          drawFastHLine(seg_start, y, x - seg_start, color);
          was_inside = false;  // must clear before break — loop won't reach the assignment below
          break;               // stadium is convex — no more inside pixels this row
        }
        was_inside = inside;
      }
      if (was_inside && seg_start >= 0)
        drawFastHLine(seg_start, y, xmax - seg_start + 1, color);
    }
  }

  void drawTextCentered(String text, int x, int y, bool color = WHITE, bool bg = false, bool border = false, int border_size = 2) {
    int16_t x1, y1;
    uint16_t w, h;
    // Measure at a safe y to avoid negative coordinate artefacts.
    const int16_t measure_y = 100;
    getTextBounds(text, 0, measure_y, &x1, &y1, &w, &h);
    const int16_t ascent = measure_y - y1;
    // (x, y) = center of the text bounding box; cursor accounts for bearing.
    const int16_t cx = x - w / 2 - x1;
    const int16_t cy = y - h / 2 + ascent;
    if (bg) {
      fillRect(x - w / 2 - border_size, y - h / 2 - border_size, w + border_size * 2, h + border_size * 2, !color);
    }
    if (border) {
      drawRect(x - w / 2 - border_size, y - h / 2 - border_size, w + border_size * 2, h + border_size * 2, color);
    }
    setCursor(cx, cy);
    setTextColor(color);
    print(text);
  }
};

ExtGFXcanvas1 display(SCREEN_WIDTH, SCREEN_HEIGHT);

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
static uint8_t* _shadow_buffer = nullptr;
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
  display.setFont(&FreeMonoBold9pt7b);
  // Get the font height by measuring text bounds for "A"
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds("A", 0, 0, &x1, &y1, &w, &h);
  font_height = h;
  Serial.println(font_height);

  size_t bufSize = (SCREEN_WIDTH * SCREEN_HEIGHT) / 8;
  _shadow_buffer = (uint8_t*)malloc(bufSize);
  if (_shadow_buffer)
    memset(_shadow_buffer, 0xff, bufSize);  // all white

  pinMode(PIN_DISPLAY_ON, OUTPUT);
  digitalWrite(PIN_DISPLAY_ON, true);
}

// Sends only lines that changed since the last call. VCOM is toggled every
// call as required by the Sharp spec (prevents DC bias damage).
void display_refresh_dirty() {
  if (!_shadow_buffer) {
    // Fallback: send all lines without dirty tracking
    uint8_t* buf = display.getBuffer();
    const uint16_t bpl = SCREEN_WIDTH / 8;
    SPI.beginTransaction(SPISettings(8000000, LSBFIRST, SPI_MODE0));
    digitalWrite(PIN_DISPLAY_SS, HIGH);
    SPI.write(_display_vcom | SHARPMEM_BIT_WRITECMD);
    _display_vcom = _display_vcom ? 0x00 : SHARPMEM_BIT_VCOM;
    uint8_t line_buf[bpl];
    for (uint16_t line = 0; line < SCREEN_HEIGHT; line++) {
      uint8_t* src = buf + line * bpl;
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

  uint8_t* buf = display.getBuffer();
  const uint16_t bpl = SCREEN_WIDTH / 8;  // bytes per line

  SPI.beginTransaction(SPISettings(8000000, LSBFIRST, SPI_MODE0));
  digitalWrite(PIN_DISPLAY_SS, HIGH);

  SPI.write(_display_vcom | SHARPMEM_BIT_WRITECMD);
  _display_vcom = _display_vcom ? 0x00 : SHARPMEM_BIT_VCOM;

  uint8_t line_buf[bpl];
  for (uint16_t line = 0; line < SCREEN_HEIGHT; line++) {
    uint8_t* cur = buf + line * bpl;
    uint8_t* shd = _shadow_buffer + line * bpl;
    if (memcmp(cur, shd, bpl) == 0)
      continue;

    for (uint16_t b = 0; b < bpl; b++)
      line_buf[b] = _reverse_byte(cur[b]);
    SPI.write(line + 1);            // 1-indexed line address
    SPI.writeBytes(line_buf, bpl);  // bit-reversed pixel data
    SPI.write(0x00);                // line trailer
    memcpy(shd, cur, bpl);          // shadow tracks raw canvas bytes
  }

  SPI.write(0x00);  // frame trailer
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
  display.setFont(&Panell_Regular9pt7b);

  // Measure at a safe y (100) to get text dimensions and ascent without
  // producing off-screen negative coordinates.
  display.getTextBounds(text, x + border, 100, &x1, &y1, &w, &h);
  int ascent = 100 - y1;  // how far text top is above the cursor (baseline)

  if (bg) {
    display.fillRect(x, y, (x1 - x) + w + border, h + border * 2, !color);
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