// ============================================================================
//  s3demo.ino — all-in-one ESP32-S3 demo firmware
//  Driven entirely over a serial terminal (ANSI UI). See README.md.
// ============================================================================
#include "config.h"
#include "term.h"
#include "rgb.h"
#include "screen.h"
#include "brand.h"
#include "io.h"
#include "bleserial.h"
#include "console.h"

static const Screen* g_cur = nullptr;
const Screen* currentScreen() { return g_cur; }

// Demo registry — order = menu order = number keys 1..N
const Screen* const SCREENS[] = {
  &screenRGB, &screenWifiScan, &screenWifiMon, &screenBLE,
  &screenWIDS, &screenHID, &screenSysInfo,
};
const int SCREEN_COUNT = sizeof(SCREENS) / sizeof(SCREENS[0]);

void gotoScreen(const Screen* s) {
  if (g_cur && g_cur->leave) g_cur->leave();
  rgb::alert(false);
  term::reset();
  term::clear();
  g_cur = s;
  if (g_cur && g_cur->enter) g_cur->enter();
}

void gotoMenu() { gotoScreen(&screenMenu); }

void setup() {
  hidBegin();        // bring up USB composite CDC + HID early, before we draw anything
  Serial.begin(SERIAL_BAUD);
  unsigned long t = millis();
  while (!Serial && millis() - t < 1500) { /* wait for USB CDC host */ }

  cfg::begin();
  rgb::begin();
  term::begin();
  io.begin();
  bleserial::begin();   // NUS peripheral: advertise "littleHAKR", own the NimBLE stack
  brand::splash();
  gotoMenu();
}

void loop() {
  // Redraw the current screen when a USB terminal or a BLE phone (re)connects, so
  // attaching after boot shows the UI immediately instead of a blank screen.
  static bool wasConn = false, wasBle = false;
  bool conn = (bool)Serial;
  bool ble  = bleserial::connected();
  if ((conn && !wasConn) || (ble && !wasBle)) {
    term::begin();
    if (g_cur && g_cur->enter) g_cur->enter();
  }
  if (ble && !wasBle) console::onConnect();   // greet the phone with a 'help' hint
  wasConn = conn;
  wasBle  = ble;

  // USB terminal: interactive single-key / arrow-key UI.
  int k = term::poll();
  if (k != key::NONE) {
    if (k == key::ESC && g_cur != &screenMenu) {
      gotoMenu();
    } else if (g_cur && g_cur->onKey) {
      g_cur->onKey(k);
    }
  }

  console::service();                // BLE phone: typed command lines
  if (g_cur && g_cur->tick) g_cur->tick();
  io.service();                      // flush a plain-text screen snapshot to BLE
  rgb::tick();
}
