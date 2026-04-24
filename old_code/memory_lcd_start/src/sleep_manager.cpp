#include "sleep_manager.h"
#include <Arduino.h>

// Internal state
static int s_displayPowerPin = -1;
static int s_switchPin = -1;
static bool s_lastSwitchState = HIGH; // using INPUT_PULLUP by default

static void setDisplayPower(bool on) {
  if (s_displayPowerPin >= 0) {
    digitalWrite(s_displayPowerPin, on ? HIGH : LOW);
  }
}

// Raw deep sleep entry that does not touch the display object.
static void goToDeepSleepRaw() {
  Serial.println("Entering deep sleep.");

  setDisplayPower(false);

  gpio_num_t wakeGpio = static_cast<gpio_num_t>(s_switchPin);
  esp_sleep_enable_ext0_wakeup(wakeGpio, 1); // wake when pin == HIGH

  delay(50);
  esp_deep_sleep_start();
}

// Common path to enter deep sleep when the switch is in "sleep" position,
// and the display has already been initialized.
static void goToDeepSleep(Adafruit_SharpMem &display) {
  Serial.println("Entering deep sleep (switch in sleep position).");

  display.clearDisplay();
  display.refresh();

  goToDeepSleepRaw();
}

void initSleep(int displayPowerPin, int switchPin, Adafruit_SharpMem &display) {
  s_displayPowerPin = displayPowerPin;
  s_switchPin = switchPin;

  pinMode(s_displayPowerPin, OUTPUT);
  pinMode(s_switchPin, INPUT_PULLUP);

  s_lastSwitchState = digitalRead(s_switchPin);

  // If the board just reset while the switch is in the sleep position (LOW),
  // immediately go back to deep sleep so the switch is always respected.
  // NOTE: At this point the display has not been .begin()'d yet, so we must
  // not call into its methods.
  if (s_lastSwitchState == LOW) {
    goToDeepSleepRaw();
  } else {
    // Awake position at boot: ensure display power is on.
    setDisplayPower(true);
  }
}

void checkButtonForSleep(Adafruit_SharpMem &display) {
  bool currentState = digitalRead(s_switchPin);

  // Only react on change of the physical switch
  if (currentState == s_lastSwitchState) {
    return;
  }

  s_lastSwitchState = currentState;

  // Define LOW as "sleep" position, HIGH as "wake" (due to INPUT_PULLUP).
  if (currentState == LOW) {
    goToDeepSleep(display);
  } else {
    setDisplayPower(true);
  }
}
