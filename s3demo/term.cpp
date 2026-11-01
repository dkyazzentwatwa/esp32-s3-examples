#include "term.h"

void term::begin() {
  cursor(false);
  clear();
}

int term::poll() {
  if (!io.available()) return key::NONE;
  int c = io.read();
  if (c == 27) {                         // ESC — possibly an arrow sequence
    unsigned long t = millis();
    while (!io.available() && millis() - t < 20) { /* wait briefly */ }
    if (!io.available()) return key::ESC;
    int c1 = io.read();
    if (c1 == '[' || c1 == 'O') {
      unsigned long t2 = millis();
      while (!io.available() && millis() - t2 < 20) { }
      if (!io.available()) return key::ESC;
      switch (io.read()) {
        case 'A': return key::UP;
        case 'B': return key::DOWN;
        case 'C': return key::RIGHT;
        case 'D': return key::LEFT;
        default:  return key::NONE;
      }
    }
    return key::ESC;
  }
  if (c == '\r' || c == '\n') return key::ENTER;
  return c;
}

void term::clear()            { io.print("\x1b[2J\x1b[H"); }
void term::home()             { io.print("\x1b[H"); }
void term::reset()            { io.print("\x1b[0m"); }
void term::cursor(bool on)    { io.print(on ? "\x1b[?25h" : "\x1b[?25l"); }
void term::moveTo(int r,int c){ io.printf("\x1b[%d;%dH", r, c); }
void term::fg(RGBc c)         { io.printf("\x1b[38;2;%u;%u;%um", c.r, c.g, c.b); }
void term::bg(RGBc c)         { io.printf("\x1b[48;2;%u;%u;%um", c.r, c.g, c.b); }
void term::bold(bool on)      { io.print(on ? "\x1b[1m" : "\x1b[22m"); }

void term::at(int row, int col, const char* text) {
  moveTo(row, col);
  io.print(text);
}

void term::clearLine(int row) {
  moveTo(row, 1);
  io.print("\x1b[2K");
}

void term::center(int row, const char* text, RGBc c) {
  int len = strlen(text);
  int col = (SCREEN_W - len) / 2;
  if (col < 1) col = 1;
  fg(c);
  moveTo(row, col);
  io.print(text);
  reset();
}

void term::box(int row, int col, int h, int w, RGBc c) {
  fg(c);
  moveTo(row, col); io.print("┌");
  for (int i = 0; i < w - 2; i++) io.print("─");
  io.print("┐");
  for (int r = 1; r < h - 1; r++) {
    moveTo(row + r, col);         io.print("│");
    moveTo(row + r, col + w - 1); io.print("│");
  }
  moveTo(row + h - 1, col); io.print("└");
  for (int i = 0; i < w - 2; i++) io.print("─");
  io.print("┘");
  reset();
}

void term::bar(int row, int col, int width, float pct, RGBc c) {
  if (pct < 0) pct = 0;
  if (pct > 1) pct = 1;
  int fill = (int)(pct * width + 0.5f);
  moveTo(row, col);
  fg(c);
  for (int i = 0; i < fill; i++) io.print("█");   // full block
  fg(pal::dim);
  for (int i = fill; i < width; i++) io.print("·"); // middle dot
  reset();
}

void term::header(const char* title, const char* tag) {
  // littleHAKR brand bar across the full width.
  // \x1b[K erases to end-of-line using the current bg, so the bar fills the
  // row without any multibyte width math.
  moveTo(1, 1);
  bg(pal::pink); fg(pal::bg); bold(true);
  io.print(" ⚡ littleHAKR  ");
  fg(pal::bg);
  io.print(title);
  io.print("\x1b[K");
  if (tag && *tag) {
    moveTo(1, SCREEN_W - (int)strlen(tag) - 3);
    bg(pal::pink); fg(pal::bg); bold(true);
    io.printf("[%s] ", tag);
  }
  reset();
  moveTo(2, 1);  // breathing room
}

void term::footer(const char* hints) {
  moveTo(SCREEN_H, 1);
  bg(pal::dim); fg(pal::bg);
  io.print(" ");
  io.print(hints);
  io.print("\x1b[K");
  // right-aligned handle
  moveTo(SCREEN_H, SCREEN_W - 12);
  bg(pal::dim); fg(pal::cyan); bold(true);
  io.print("@littlehakr ");
  reset();
}
