#include "battery_monitor.h"
#include <Adafruit_SharpMem.h>
#include <Fonts/FreeMono9pt7b.h>

static int s_vbatPin = -1;
static BatteryStatus status = {0.0f, 0, false, false};

void initBatteryMonitor(int vbatPin) {
  s_vbatPin = vbatPin;
  pinMode(s_vbatPin, INPUT);
}

void updateBatteryStatus() {
  // HUZZAH32: voltage divider halves VBAT, so multiply by 2
  float mv = analogReadMilliVolts(s_vbatPin);
  status.voltageV = (mv * 2.0f) / 1000.0f;

  // LiPo: linear approx 3.2V = 0%, 4.2V = 100%
  float pct =
      (status.voltageV - LIPO_EMPTY_V) / (LIPO_FULL_V - LIPO_EMPTY_V) * 100.0f;
  status.percent = (int)(pct < 0 ? 0 : (pct > 100 ? 100 : pct));
}

BatteryStatus getBatteryStatus() { return status; }

void drawBatteryStatus(Adafruit_SharpMem &display) {
  // gfx.setTextSize(3);
  display.setCursor(2, 2);
  display.setFont(&FreeMono9pt7b);
  display.setTextColor(1);

  display.fillRect(2, 2, 100, 12, 0);

  display.setCursor(4, 12);

  display.print(status.voltageV, 2);
  display.print("V ");
  display.print(status.percent);
  display.print("%");
}
