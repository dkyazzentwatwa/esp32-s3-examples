#include "console.h"
#include "screen.h"
#include "io.h"
#include "bleserial.h"
#include <ctype.h>

// Primary nav word for each demo, in SCREENS[] order (RGB, Wi-Fi, Monitor, BLE,
// WIDS, HID, SysInfo). Used for the help listing.
static const char* const PRIMARY[] = { "rgb", "wifi", "mon", "ble", "wids", "hid", "sys" };

// Nav words (incl. synonyms) -> SCREENS[] index.
struct Nav { const char* w; int i; };
static const Nav NAV[] = {
  {"rgb",0},{"light",0},{"led",0},
  {"wifi",1},{"scan",1},{"networks",1},
  {"mon",2},{"monitor",2},{"packets",2},{"sniff",2},
  {"ble",3},{"bt",3},
  {"wids",4},{"defense",4},{"defence",4},
  {"hid",5},{"payloads",5},{"payload",5},
  {"sys",6},{"info",6},{"system",6},
};

static void say(const char* s, int n) { if (bleserial::connected()) bleserial::send((const uint8_t*)s, n); }
static void say(const char* s)        { say(s, strlen(s)); }

static void sendHelp() {
  static char b[900];
  int n = 0;
  n += snprintf(b + n, sizeof(b) - n, "\r\n== littleHAKR console ==\r\n");
  n += snprintf(b + n, sizeof(b) - n, "type a command + Enter\r\n\r\nDEMOS:\r\n");
  for (int i = 0; i < SCREEN_COUNT; i++)
    n += snprintf(b + n, sizeof(b) - n, "  %d %-5s %s\r\n", i + 1, PRIMARY[i], SCREENS[i]->title);
  n += snprintf(b + n, sizeof(b) - n, "\r\nGLOBAL:\r\n");
  n += snprintf(b + n, sizeof(b) - n, "  help      this list\r\n");
  n += snprintf(b + n, sizeof(b) - n, "  menu      main menu\r\n");
  n += snprintf(b + n, sizeof(b) - n, "  refresh   redraw screen\r\n");

  const Screen* cur = currentScreen();
  if (cur && cur->nCmds > 0) {
    n += snprintf(b + n, sizeof(b) - n, "\r\n%s:\r\n", cur->title);
    for (int i = 0; i < cur->nCmds; i++)
      n += snprintf(b + n, sizeof(b) - n, "  %-8s %s\r\n", cur->cmds[i].word, cur->cmds[i].help);
  }
  say(b, n);
}

// Lowercase + trim a line into dst.
static void normalize(const char* src, char* dst, size_t cap) {
  while (*src == ' ' || *src == '\t') src++;
  size_t n = 0;
  while (*src && n < cap - 1) dst[n++] = (char)tolower((unsigned char)*src++);
  while (n > 0 && (dst[n - 1] == ' ' || dst[n - 1] == '\t')) n--;
  dst[n] = 0;
}

static void handle(const char* raw) {
  char cmd[96];
  normalize(raw, cmd, sizeof(cmd));

  if (cmd[0] == 0) { io.forceFlush(); return; }                 // bare Enter = redraw

  if (!strcmp(cmd, "help") || !strcmp(cmd, "?") || !strcmp(cmd, "h")
      || !strcmp(cmd, "commands")) { sendHelp(); return; }

  if (!strcmp(cmd, "menu") || !strcmp(cmd, "back") || !strcmp(cmd, "m")
      || !strcmp(cmd, "b") || !strcmp(cmd, "home")) { gotoMenu(); io.markActivity(); return; }

  if (!strcmp(cmd, "refresh") || !strcmp(cmd, "redraw") || !strcmp(cmd, "r?")) {
    io.forceFlush(); return;
  }

  // Navigate to a demo by name (works from any screen).
  for (unsigned i = 0; i < sizeof(NAV) / sizeof(NAV[0]); i++)
    if (!strcmp(cmd, NAV[i].w)) { gotoScreen(SCREENS[NAV[i].i]); io.markActivity(); return; }

  const Screen* cur = currentScreen();

  // Let the current screen's custom handler try first (e.g. HID numeric select).
  if (cur && cur->onCmd && cur->onCmd(cmd)) { io.markActivity(); return; }

  // Match the screen's simple command table (word -> key).
  if (cur && cur->cmds) {
    for (int i = 0; i < cur->nCmds; i++)
      if (cur->cmds[i].key && !strcmp(cmd, cur->cmds[i].word)) {
        if (cur->onKey) cur->onKey(cur->cmds[i].key);
        io.markActivity(); return;
      }
  }

  // A bare number opens a demo by position.
  if (cmd[1] == 0 && cmd[0] >= '1' && cmd[0] <= '0' + SCREEN_COUNT) {
    gotoScreen(SCREENS[cmd[0] - '1']); io.markActivity(); return;
  }

  // A lone character falls through as a raw keypress (power-user escape hatch).
  if (cmd[1] == 0 && cur && cur->onKey) { cur->onKey(cmd[0]); io.markActivity(); return; }

  say("unknown command - type 'help'\r\n");
}

namespace console {

void onConnect() {
  say("\r\n== littleHAKR ==\r\nphone console ready. type 'help' + Enter.\r\n");
}

void service() {
  char line[96];
  while (io.bleNextLine(line, sizeof(line))) handle(line);
}

} // namespace console
