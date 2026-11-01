#include "screen.h"
#include "term.h"
#include "rgb.h"
#include "config.h"
#include "esp_mac.h"

static unsigned long lastDraw = 0;

static void row(int r, const char* label, const char* value) {
  term::moveTo(r, 7); term::fg(pal::dim);  io.printf("%-16s", label);
  term::fg(pal::fg);  term::bold(true);     io.print(value);
  term::reset();
}

static void render() {
  uint8_t mac[6] = {0};
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  char macs[18];
  snprintf(macs, sizeof(macs), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  char buf[48];
  term::box(5, 4, 15, 72, pal::accent);

  snprintf(buf, sizeof(buf), "%s  (%d cores @ %d MHz)",
           ESP.getChipModel(), ESP.getChipCores(), ESP.getCpuFreqMHz());
  row(6, "Chip", buf);

  snprintf(buf, sizeof(buf), "%u MB", (unsigned)(ESP.getFlashChipSize() / (1024 * 1024)));
  row(7, "Flash", buf);

  size_t psram = ESP.getPsramSize();
  if (psram) snprintf(buf, sizeof(buf), "%u MB", (unsigned)(psram / (1024 * 1024)));
  else       snprintf(buf, sizeof(buf), "none");
  row(8, "PSRAM", buf);

  snprintf(buf, sizeof(buf), "%.1f C", temperatureRead());
  row(9, "Internal temp", buf);

  snprintf(buf, sizeof(buf), "%u KB free / %u KB total",
           (unsigned)(ESP.getFreeHeap() / 1024), (unsigned)(ESP.getHeapSize() / 1024));
  row(10, "Heap", buf);

  row(11, "Wi-Fi MAC", macs);
  row(12, "SDK", ESP.getSdkVersion());

  unsigned long up = millis() / 1000;
  snprintf(buf, sizeof(buf), "%lu:%02lu:%02lu", up / 3600, (up % 3600) / 60, up % 60);
  row(13, "Uptime", buf);

  snprintf(buf, sizeof(buf), "GPIO%u  (press p to switch)", cfg::ledPin());
  row(14, "RGB LED pin", buf);

  term::moveTo(18, 7); term::fg(pal::dim);
  io.print("t = blink LED test    p = toggle LED pin (GPIO48 ↔ GPIO38)");
  term::reset();
}

static void enter() {
  term::clear(); term::header("SYSTEM INFO", "stats");
  term::footer("  t LED test   p toggle LED pin      Esc menu"); render(); }

static void tick() {
  if (millis() - lastDraw > 500) { render(); lastDraw = millis(); }
}

static void leave() { }

static void onKey(int k) {
  if (k == 't' || k == 'T') {                 // visible blink test (brief, blocking)
    const RGBc seq[3] = { {255,0,0}, {0,255,0}, {0,80,255} };
    for (int i = 0; i < 3; i++) { rgb::showNow(seq[i]); delay(400); }
    rgb::setEffect(rgb::RAINBOW);
  } else if (k == 'p' || k == 'P') {
    uint8_t np = (cfg::ledPin() == 48) ? 38 : 48;
    cfg::setLedPin(np);
    rgb::setPin(np);
    render();
  }
}

static const Cmd cmds[] = {
  { "test", 't', "blink the LED (red/green/blue)" },
  { "pin",  'p', "toggle LED data pin (GPIO48/38)" },
};

const Screen screenSysInfo = { "System Info", "stats", enter, tick, leave, onKey, SCMDS(cmds), nullptr };
