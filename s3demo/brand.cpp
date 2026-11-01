#include "brand.h"
#include "term.h"
#include "rgb.h"

// "little HAKR" wordmark in a small box-drawing figlet (~30 columns wide).
static const char* LOGO[3] = {
  "╦  ╦╔╦╗╔╦╗╦  ╔═╗  ╦ ╦╔═╗╦╔═╦═╗",
  "║  ║ ║  ║ ║  ╠═   ╠═╣╠═╣╠╩╗╠╦╝",
  "╩═╝╩ ╩  ╩ ╩═╝╚═╝  ╩ ╩╩ ╩╩ ╚╩╚═",
};
static const int LOGO_COL = 25;   // (80 - 30) / 2, centered

void brand::logo(int topRow) {
  RGBc grad[3] = { pal::pink, {210, 80, 220}, pal::cyan };
  for (int i = 0; i < 3; i++) {
    term::moveTo(topRow + i, LOGO_COL);
    term::fg(grad[i]); term::bold(true);
    io.print(LOGO[i]);
  }
  term::reset();
}

void brand::splash() {
  term::clear();
  brand::logo(7);
  term::center(11, "no solder · all signal", pal::dim);
  term::center(13, "ESP32-S3 DEMO CONSOLE", pal::cyan);

  // neon loading sweep, with the onboard LED riding along
  RGBc led[4] = { pal::pink, pal::purple, pal::cyan, pal::lime };
  for (int p = 0; p <= 24; p++) {
    term::bar(15, 28, 24, p / 24.0f, p & 1 ? pal::pink : pal::cyan);
    rgb::showNow(led[(p / 3) % 4]);
    delay(45);
  }
  term::center(17, "@littlehakr", pal::pink);
  delay(500);
}
