#pragma once
#include <Arduino.h>
#include "io.h"   // UI writes go through `io` (USB ANSI + BLE text), not Serial

// Fixed virtual screen size we lay out against (works in any 80x24+ terminal).
static const int SCREEN_W = 80;
static const int SCREEN_H = 24;

struct RGBc { uint8_t r, g, b; };

// Key codes returned by term::poll()
namespace key {
  enum {
    NONE  = -1,
    ENTER = 13,
    ESC   = 27,
    UP    = 256, DOWN, LEFT, RIGHT,
  };
}

// UI palette (GitHub-dark-ish, reads well on camera)
namespace pal {
  constexpr RGBc bg     = { 13,  17,  23};
  constexpr RGBc fg     = {201, 209, 217};
  constexpr RGBc accent = { 88, 166, 255};
  constexpr RGBc good   = { 63, 185,  80};
  constexpr RGBc warn   = {210, 153,  34};
  constexpr RGBc bad    = {248,  81,  73};
  constexpr RGBc dim    = {110, 118, 129};
  constexpr RGBc purple = {188, 140, 255};
  // littleHAKR neon brand accents (used for chrome; data stays semantic)
  constexpr RGBc pink   = {255,  55, 160};
  constexpr RGBc cyan   = { 80, 220, 255};
  constexpr RGBc lime   = {166, 255,  80};
}

namespace term {
  void begin();
  int  poll();                                  // non-blocking key; key::NONE if none

  // low-level output
  void clear();
  void home();
  void cursor(bool on);
  void moveTo(int row, int col);                // 1-based
  void reset();
  void fg(RGBc c);
  void bg(RGBc c);
  void bold(bool on = true);

  // layout helpers (1-based rows/cols)
  void at(int row, int col, const char* text);
  void clearLine(int row);
  void center(int row, const char* text, RGBc c);
  void box(int row, int col, int h, int w, RGBc c);
  void bar(int row, int col, int width, float pct, RGBc c);  // filled bar graph

  // shared chrome
  void header(const char* title, const char* tag);
  void footer(const char* hints);               // bottom hint line
}
