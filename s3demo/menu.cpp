#include "screen.h"
#include "term.h"
#include "brand.h"
#include "config.h"

static int sel = 0;

static void render() {
  term::clear();
  brand::logo(2);
  term::center(6, "no solder · all signal   ·   v" S3DEMO_VERSION, pal::dim);

  for (int i = 0; i < SCREEN_COUNT; i++) {
    int row = 8 + i;
    term::moveTo(row, 8);
    if (i == sel) {
      term::bg(pal::pink); term::fg(pal::bg); term::bold(true);
      char line[40];
      snprintf(line, sizeof(line), " %d  %-26s ", i + 1, SCREENS[i]->title);
      io.print(line);
      term::reset();
    } else {
      term::fg(pal::cyan); term::bold(true);
      io.printf(" %d  ", i + 1);
      term::reset();
      term::fg(pal::fg);
      io.printf("%-26s", SCREENS[i]->title);
      term::reset();
    }
    term::fg(pal::dim); term::moveTo(row, 46);
    io.printf("[%s]", SCREENS[i]->tag);
    term::reset();
  }
  term::footer("  ↑/↓ or 1-7 to select   ↵ launch     Esc returns here");
}

static void enter() { sel = 0; render(); }
static void tick()  { }
static void leave() { }

static void onKey(int k) {
  if      (k == key::UP)    { sel = (sel + SCREEN_COUNT - 1) % SCREEN_COUNT; render(); }
  else if (k == key::DOWN)  { sel = (sel + 1) % SCREEN_COUNT; render(); }
  else if (k == key::ENTER) { gotoScreen(SCREENS[sel]); }
  else if (k >= '1' && k < '1' + SCREEN_COUNT) { gotoScreen(SCREENS[k - '1']); }
}

const Screen screenMenu = { "ESP32-S3 DEMO CONSOLE", "menu", enter, tick, leave, onKey };
