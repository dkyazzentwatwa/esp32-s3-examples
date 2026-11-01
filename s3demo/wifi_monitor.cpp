#include "screen.h"
#include "term.h"
#include "rgb.h"
#include <WiFi.h>
#include "esp_wifi.h"

// PASSIVE, receive-only 802.11 frame counter. It never transmits.
static volatile uint32_t chCount[14] = {0};
static volatile uint32_t totAll = 0, totMgmt = 0, totData = 0, totCtrl = 0, totBeacon = 0;
static portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

static int curCh = 1;
static uint32_t lastAll = 0;
static float pps = 0;
static unsigned long lastHop = 0, lastDraw = 0, lastPps = 0;

static void sniff(void* buf, wifi_promiscuous_pkt_type_t type) {
  const wifi_promiscuous_pkt_t* p = (const wifi_promiscuous_pkt_t*)buf;
  const uint8_t* fr = p->payload;
  uint8_t ftype = (fr[0] & 0x0C) >> 2;      // 0=mgmt 1=ctrl 2=data
  uint8_t fsub  = (fr[0] & 0xF0) >> 4;
  portENTER_CRITICAL(&mux);
  totAll++;
  if (curCh >= 1 && curCh <= 13) chCount[curCh]++;
  if      (ftype == 0) { totMgmt++; if (fsub == 8) totBeacon++; }
  else if (ftype == 1) { totCtrl++; }
  else if (ftype == 2) { totData++; }
  portEXIT_CRITICAL(&mux);
}

static void resetCounts() {
  portENTER_CRITICAL(&mux);
  for (int i = 0; i < 14; i++) chCount[i] = 0;
  totAll = totMgmt = totData = totCtrl = totBeacon = 0;
  portEXIT_CRITICAL(&mux);
  lastAll = 0; pps = 0;
}

static void enter() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  esp_wifi_set_promiscuous(false);
  resetCounts();
  esp_wifi_set_promiscuous_rx_cb(&sniff);
  esp_wifi_set_promiscuous(true);
  curCh = 1;
  esp_wifi_set_channel(curCh, WIFI_SECOND_CHAN_NONE);

  term::clear();
  term::header("WI-FI PACKET MONITOR", "sniff");
  term::center(4, "passive promiscuous capture — receive-only, counts only", pal::dim);
  term::footer("  hopping channels 1-13 · counting frames        Esc menu");
}

static void render() {
  uint32_t ch[14], all, mg, dt, ct, bc;
  portENTER_CRITICAL(&mux);
  for (int i = 0; i < 14; i++) ch[i] = chCount[i];
  all = totAll; mg = totMgmt; dt = totData; ct = totCtrl; bc = totBeacon;
  portEXIT_CRITICAL(&mux);

  // top counters
  term::moveTo(6, 4); term::fg(pal::fg); term::bold(true);
  io.printf("packets/sec: %6.0f", pps); term::reset();
  term::moveTo(6, 32); term::fg(pal::accent); io.printf("total %lu", (unsigned long)all); term::reset();

  term::moveTo(7, 4);  term::fg(pal::purple); io.printf("beacon %-8lu", (unsigned long)bc);
  term::fg(pal::good); io.printf("mgmt %-8lu", (unsigned long)mg);
  term::fg(pal::warn); io.printf("data %-8lu", (unsigned long)dt);
  term::fg(pal::dim);  io.printf("ctrl %-8lu", (unsigned long)ct);
  term::reset();

  // per-channel bars
  uint32_t mx = 1;
  for (int c = 1; c <= 13; c++) if (ch[c] > mx) mx = ch[c];
  for (int c = 1; c <= 13; c++) {
    int row = 9 + (c - 1);
    term::moveTo(row, 4); term::fg(c == curCh ? pal::accent : pal::dim);
    io.printf("ch %2d ", c); term::reset();
    term::bar(row, 11, 40, (float)ch[c] / (float)mx, c == curCh ? pal::accent : pal::good);
    term::moveTo(row, 54); term::fg(pal::dim); io.printf(" %lu   ", (unsigned long)ch[c]);
    term::reset();
  }
}

static void tick() {
  unsigned long now = millis();
  if (now - lastHop > 280) {                     // channel hop
    curCh = (curCh % 13) + 1;
    esp_wifi_set_channel(curCh, WIFI_SECOND_CHAN_NONE);
    lastHop = now;
  }
  if (now - lastPps >= 1000) {                    // packets/sec
    uint32_t a; portENTER_CRITICAL(&mux); a = totAll; portEXIT_CRITICAL(&mux);
    pps = (float)(a - lastAll);
    if (a != lastAll) rgb::flash(pal::good);
    lastAll = a;
    lastPps = now;
  }
  if (now - lastDraw > 150) { render(); lastDraw = now; }
}

static void leave() {
  esp_wifi_set_promiscuous(false);
  esp_wifi_set_promiscuous_rx_cb(NULL);
  WiFi.mode(WIFI_OFF);
}

static void onKey(int k) { if (k == 'r' || k == 'R') resetCounts(); }

static const Cmd cmds[] = { { "reset", 'r', "reset packet counters" } };

const Screen screenWifiMon = { "Wi-Fi Packet Monitor", "sniff", enter, tick, leave, onKey, SCMDS(cmds), nullptr };
