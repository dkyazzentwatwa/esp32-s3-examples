#include "screen.h"
#include "term.h"
#include "rgb.h"
#include "config.h"

static unsigned long lastDraw = 0;

static void drawStatics() {
  term::clear();
  term::header("RGB LIGHT SHOW", "visual");
  term::center(4, "the onboard WS2812 mirrored on-screen — this band IS the demo", pal::dim);
  term::moveTo(18, 4); term::fg(pal::dim);
  io.print("Bare board has 1 LED, so the colour band above is the star of the shot.");
  term::moveTo(19, 4);
  io.print("LED dark? Press 'p' to switch the data pin (GPIO48 ↔ GPIO38).");
  term::reset();
  term::footer("  ←/→ effect   +/- speed   [ ] brightness   p pin   Esc menu");
}

static void drawLive() {
  // big colour swatch mirroring the LED
  RGBc c = rgb::color();
  term::bg(c);
  for (int r = 6; r <= 12; r++) {
    term::moveTo(r, 4);
    for (int i = 0; i < 73; i++) io.print(" ");
  }
  term::reset();

  term::moveTo(14, 4); term::fg(pal::fg); term::bold(true);
  io.printf("Effect: %-14s", rgb::effectName(rgb::effect()));
  term::reset();
  term::fg(pal::dim); io.printf("  rgb(%3u,%3u,%3u)   pin GPIO%u", c.r, c.g, c.b, cfg::ledPin());
  term::reset();

  term::moveTo(15, 4); term::fg(pal::dim); io.print("Speed      "); term::reset();
  term::bar(15, 16, 24, rgb::speed() / 20.0f, pal::accent);
  term::moveTo(16, 4); term::fg(pal::dim); io.print("Brightness "); term::reset();
  term::bar(16, 16, 24, rgb::brightness() / 255.0f, pal::warn);
}

static void enter() {
  io.setBleAnimated(true);    // live LED readout animates ~20fps — don't stream it to BLE
  if (rgb::effect() == rgb::OFF) rgb::setEffect(rgb::RAINBOW);
  drawStatics();
}
static void tick() {
  unsigned long now = millis();
  if (now - lastDraw < 50) return;
  lastDraw = now;
  drawLive();
}
static void leave() { io.setBleAnimated(false); }

static void onKey(int k) {
  switch (k) {
    case key::RIGHT: case 'n': rgb::nextEffect(); break;
    case key::LEFT:  case 'N': rgb::prevEffect(); break;
    case '+': case '=':        rgb::setSpeed(+1); break;
    case '-': case '_':        rgb::setSpeed(-1); break;
    case ']':                  rgb::setBrightness(+15); break;
    case '[':                  rgb::setBrightness(-15); break;
    case 'p': case 'P': {
      uint8_t np = (cfg::ledPin() == 48) ? 38 : 48;
      cfg::setLedPin(np);
      rgb::setPin(np);
      break;
    }
    case ' ': rgb::nextEffect(); break;
  }
}

static const Cmd cmds[] = {
  { "effect", 'n', "next effect" },
  { "prev",   'N', "previous effect" },
  { "faster", '+', "speed up" },
  { "slower", '-', "slow down" },
  { "bright", ']', "brighter" },
  { "dim",    '[', "dimmer" },
  { "pin",    'p', "toggle LED data pin (GPIO48/38)" },
};

const Screen screenRGB = { "RGB Light Show", "visual", enter, tick, leave, onKey, SCMDS(cmds), nullptr };
