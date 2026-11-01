#include "screen.h"
#include "term.h"
#include "rgb.h"
#include <NimBLEDevice.h>

struct Dev { char mac[18]; char name[33]; int rssi; unsigned long seen; };
static const int MAXDEV = 24;
static Dev devs[MAXDEV];
static int nDev = 0;
static SemaphoreHandle_t mtx = nullptr;
static unsigned long lastDraw = 0;

static void upsert(const char* mac, const char* name, int rssi) {
  if (!mtx || xSemaphoreTake(mtx, 0) != pdTRUE) return;
  int idx = -1;
  for (int i = 0; i < nDev; i++) if (strcmp(devs[i].mac, mac) == 0) { idx = i; break; }
  if (idx < 0) {
    if (nDev < MAXDEV) idx = nDev++;
    else {                                   // replace the stalest entry
      idx = 0;
      for (int i = 1; i < nDev; i++) if (devs[i].seen < devs[idx].seen) idx = i;
    }
    strncpy(devs[idx].mac, mac, sizeof(devs[idx].mac) - 1);
    devs[idx].mac[sizeof(devs[idx].mac) - 1] = 0;
  }
  strncpy(devs[idx].name, (name && *name) ? name : "(no name)", sizeof(devs[idx].name) - 1);
  devs[idx].name[sizeof(devs[idx].name) - 1] = 0;
  devs[idx].rssi = rssi;
  devs[idx].seen = millis();
  xSemaphoreGive(mtx);
}

class Cbs : public NimBLEScanCallbacks {
  void onResult(const NimBLEAdvertisedDevice* d) override {
    std::string mac  = d->getAddress().toString();
    std::string name = d->getName();
    upsert(mac.c_str(), name.c_str(), d->getRSSI());
  }
};
static Cbs cbs;

static RGBc rssiColor(int r) {
  if (r >= -60) return pal::good;
  if (r >= -80) return pal::warn;
  return pal::bad;
}

static void enter() {
  if (!mtx) mtx = xSemaphoreCreateMutex();
  nDev = 0;
  // NimBLE is brought up once at boot by bleserial::begin() (it owns the stack so
  // the NUS peripheral keeps advertising). We just drive the scanner here; NimBLE
  // runs central (scan) + peripheral (NUS) concurrently.
  NimBLEScan* scan = NimBLEDevice::getScan();
  scan->setScanCallbacks(&cbs, false);
  scan->setActiveScan(true);
  scan->start(0);                            // 0 = scan continuously

  term::clear();
  term::header("BLE SCANNER", "recon");
  term::moveTo(4, 4); term::fg(pal::dim); term::bold(true);
  io.printf("%-20s %-18s %-18s %s", "NAME", "MAC", "SIGNAL", "dBm");
  term::reset();
  term::footer("  phones · watches · earbuds nearby      Esc menu");
}

static void render() {
  if (!mtx) return;
  Dev snap[MAXDEV]; int n;
  xSemaphoreTake(mtx, portMAX_DELAY);
  n = nDev;
  for (int i = 0; i < n; i++) snap[i] = devs[i];
  xSemaphoreGive(mtx);

  for (int i = 0; i < n; i++)                 // sort by RSSI desc
    for (int j = i + 1; j < n; j++)
      if (snap[j].rssi > snap[i].rssi) { Dev t = snap[i]; snap[i] = snap[j]; snap[j] = t; }

  int shown = n > 16 ? 16 : n;
  for (int i = 0; i < shown; i++) {
    int row = 5 + i;
    char nm[21]; strncpy(nm, snap[i].name, 20); nm[20] = 0;
    term::clearLine(row);
    term::moveTo(row, 4); term::fg(pal::fg);
    io.printf("%-20s %-18s ", nm, snap[i].mac);
    term::reset();
    term::bar(row, 44, 14, (snap[i].rssi + 100) / 60.0f, rssiColor(snap[i].rssi));
    term::moveTo(row, 60); term::fg(pal::dim); io.printf("%d  ", snap[i].rssi);
    term::reset();
  }
  for (int i = shown; i < 16; i++) term::clearLine(5 + i);

  term::moveTo(22, 4); term::fg(pal::accent); term::bold(true);
  io.printf("%d devices in range", n); term::reset();
  if (n > 0) rgb::solid(rssiColor(snap[0].rssi));
}

static void tick() {
  if (millis() - lastDraw > 400) { render(); lastDraw = millis(); }
}

static void leave() {
  NimBLEScan* scan = NimBLEDevice::getScan();
  if (scan) scan->stop();                      // stop scanning only — leave NUS up
}

static void onKey(int k) { if (k == 'r' || k == 'R') nDev = 0; }

static const Cmd cmds[] = { { "clear", 'r', "clear the device list" } };

const Screen screenBLE = { "BLE Scanner", "recon", enter, tick, leave, onKey, SCMDS(cmds), nullptr };
