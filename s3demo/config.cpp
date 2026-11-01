#include "config.h"
#include <Preferences.h>

static Preferences prefs;
static uint8_t g_ledPin = DEFAULT_LED_PIN;

void cfg::begin() {
  prefs.begin("s3demo", false);
  g_ledPin = prefs.getUChar("ledpin", DEFAULT_LED_PIN);
}

uint8_t cfg::ledPin() { return g_ledPin; }

void cfg::setLedPin(uint8_t pin) {
  g_ledPin = pin;
  prefs.putUChar("ledpin", pin);
}
