#pragma once
#include <Arduino.h>

// littleHAKR branding for the serial UI.
namespace brand {
  void logo(int topRow);     // draws the 3-line wordmark, neon-gradient, centered
  void splash();             // full boot splash (blocking, ~1.5s) — call once at startup
}
