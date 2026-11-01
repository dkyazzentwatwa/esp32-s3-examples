#pragma once
#include <Arduino.h>
#include "term.h"   // RGBc

namespace rgb {
  enum Effect { OFF, SOLID, RAINBOW, BREATHE, STROBE, WIPE, POLICE, SPARKLE, N_EFFECTS };

  void begin();                 // init on cfg::ledPin()
  void setPin(uint8_t pin);     // re-init on a new data pin (for the 48 vs 38 fix)
  void tick();                  // advance animation — call every loop()

  void   setEffect(Effect e);
  Effect effect();
  const char* effectName(Effect e);
  void nextEffect();
  void prevEffect();

  void setSpeed(int delta);     // +/- animation speed
  int  speed();
  void setBrightness(int delta);// +/- brightness
  int  brightness();

  RGBc color();                 // current displayed color (for on-screen mirroring)
  void showNow(RGBc c);         // write a color to the LED immediately (bypasses throttle)

  // status accents any screen can use
  void solid(RGBc c);           // force a steady color (switches effect to SOLID)
  void flash(RGBc c);           // brief pulse, then resume the active effect
  void alert(bool on);          // sticky red blink (used by WIDS)
}
