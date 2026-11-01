#include "screen.h"
#include "term.h"
#include "rgb.h"
#include "config.h"
#include "payloads.h"
#include "USB.h"
#include "USBHIDKeyboard.h"

// USB-HID payload picker. The board enumerates as a real USB keyboard and types
// a selected BENIGN reel (visual theater — no real access, exfil, persistence,
// or attack). Only fires on an explicit trigger, after a visible countdown.
static USBHIDKeyboard Keyboard;

enum { LIST, DETAIL, COUNTDOWN, DONE };
static int mode = LIST;
static int sel = 0;
static int secsLeft = 0;
static unsigned long tAt = 0, sentAt = 0;
static bool lastBtn = false;
static unsigned long lastBtnMs = 0;

void hidBegin() {
  Keyboard.begin();
  USB.begin();
  pinMode(BOOT_BUTTON_PIN, INPUT_PULLUP);
}

// ---- rendering --------------------------------------------------------------
static void renderList() {
  term::clear();
  term::header("USB-HID PAYLOADS", "inject");
  term::center(4, "the board types a benign reel into a Terminal it opens for you", pal::dim);
  for (int i = 0; i < PAYLOAD_COUNT; i++) {
    int r = 6 + i;
    term::moveTo(r, 4);
    if (i == sel) { term::bg(pal::accent); term::fg(pal::bg); term::bold(true); }
    else          { term::fg(pal::fg); }
    char line[40];
    snprintf(line, sizeof(line), " %2d  %-20s ", i + 1, PAYLOADS[i].title);
    io.print(line);
    term::reset();
    term::fg(pal::dim); term::moveTo(r, 34);
    io.print(PAYLOADS[i].vibe);
    term::reset();
  }
  term::footer("  ↑/↓ select   ↵ open details   BOOT button = run selected   Esc menu");
}

static void renderDetail() {
  const Payload& p = PAYLOADS[sel];
  term::clear();
  term::header("USB-HID PAYLOAD", "inject");
  term::moveTo(4, 4); term::fg(pal::accent); term::bold(true); io.print(p.title); term::reset();
  term::moveTo(5, 4); term::fg(pal::dim); io.print(p.vibe); term::reset();

  term::box(7, 4, 5, 72, pal::warn);
  term::moveTo(8, 7); term::fg(pal::warn); term::bold(true);
  io.print("⚠  On run it types for you. It will open a Terminal and type");
  term::moveTo(9, 7); term::fg(pal::dim);
  io.printf("%d command line(s). Harmless & reversible. US keyboard layout.", p.nCmds);
  term::moveTo(10, 7);
  io.print("Keep the Terminal/desktop focused once the countdown starts.");
  term::reset();

  term::moveTo(14, 4); term::fg(pal::accent); term::bold(true);
  io.print("  ↵ or BOOT button = run (3-2-1)      ← back to list");
  term::reset();
  term::footer("  Esc menu");
}

static void renderCountdown() {
  term::moveTo(14, 4); io.print("\x1b[2K");
  term::moveTo(14, 4); term::fg(pal::bad); term::bold(true);
  io.printf("  Typing \"%s\" in %d ...  (Esc cancels)", PAYLOADS[sel].title, secsLeft);
  term::reset();
}

static void renderDone() {
  term::moveTo(14, 4); io.print("\x1b[2K");
  term::moveTo(14, 4); term::fg(pal::good); term::bold(true);
  io.print("  ✓ Sent! Check the Terminal window.");
  term::reset();
}

// ---- typing -----------------------------------------------------------------
static void openTerminalMac() {
  Keyboard.press(KEY_LEFT_GUI); Keyboard.press(' ');
  delay(60); Keyboard.releaseAll(); delay(650);
  Keyboard.print("Terminal");
  delay(350); Keyboard.write('\n'); delay(1700);
}

static void runPayload() {
  const Payload& p = PAYLOADS[sel];
  rgb::flash(pal::good);
  if (p.opensTerminal) openTerminalMac();
  for (int i = 0; i < p.nCmds; i++) {
    Keyboard.print(p.cmds[i]);
    Keyboard.write('\n');
    rgb::flash(pal::accent);
    delay(500);
  }
}

static void startCountdown() {
  if (mode == COUNTDOWN) return;
  mode = COUNTDOWN; secsLeft = 3; tAt = millis();
  renderDetail();           // ensure detail frame is up
  renderCountdown();
}

// ---- screen hooks -----------------------------------------------------------
static void enter() { mode = LIST; sel = 0; renderList(); }

static void tick() {
  unsigned long now = millis();

  bool b = (digitalRead(BOOT_BUTTON_PIN) == LOW);
  if (b && !lastBtn && now - lastBtnMs > 250) {
    if (mode == LIST || mode == DETAIL) startCountdown();
    lastBtnMs = now;
  }
  lastBtn = b;

  if (mode == COUNTDOWN && now - tAt >= 1000) {
    secsLeft--; tAt = now;
    if (secsLeft <= 0) { runPayload(); mode = DONE; sentAt = now; renderDone(); }
    else renderCountdown();
  } else if (mode == DONE && now - sentAt > 3000) {
    mode = LIST; renderList();
  }
}

static void leave() { mode = LIST; }

static void onKey(int k) {
  if (mode == LIST) {
    if      (k == key::UP)    { sel = (sel + PAYLOAD_COUNT - 1) % PAYLOAD_COUNT; renderList(); }
    else if (k == key::DOWN)  { sel = (sel + 1) % PAYLOAD_COUNT; renderList(); }
    else if (k == key::ENTER) { mode = DETAIL; renderDetail(); }
  } else if (mode == DETAIL) {
    if      (k == key::ENTER || k == ' ') startCountdown();
    else if (k == key::LEFT)              { mode = LIST; renderList(); }
  }
}

// Phone console: this screen is a LIST/DETAIL state machine, so arrow/Enter keys
// don't map to single commands — handle typed commands (and numeric selection) here.
static bool onCmd(const char* line) {
  if (!strcmp(line, "list"))                        { mode = LIST; renderList(); return true; }
  if (!strcmp(line, "next"))                        { sel = (sel + 1) % PAYLOAD_COUNT; mode = LIST; renderList(); return true; }
  if (!strcmp(line, "prev"))                        { sel = (sel + PAYLOAD_COUNT - 1) % PAYLOAD_COUNT; mode = LIST; renderList(); return true; }
  if (!strcmp(line, "open") || !strcmp(line, "details")) { mode = DETAIL; renderDetail(); return true; }
  if (!strcmp(line, "run")  || !strcmp(line, "go"))      { startCountdown(); return true; }
  if (line[0] >= '1' && line[0] <= '9') {            // pick payload by number -> details
    int n = atoi(line);
    if (n >= 1 && n <= PAYLOAD_COUNT) { sel = n - 1; mode = DETAIL; renderDetail(); return true; }
  }
  return false;
}

static const Cmd cmds[] = {
  { "list", 0, "show the payload list" },
  { "<n>",  0, "pick payload #n and show it" },
  { "next", 0, "select next payload" },
  { "prev", 0, "select previous payload" },
  { "run",  0, "run the selected payload (3-2-1)" },
};

const Screen screenHID = { "USB-HID Payloads", "inject", enter, tick, leave, onKey, SCMDS(cmds), onCmd };
