#include "screen.h"
#include "term.h"
#include "rgb.h"
#include <WiFi.h>
#include "esp_wifi.h"

// ============================================================================
//  Wireless Defense / WIDS  (DEFENSIVE, RECEIVE-ONLY)
//  Listens in promiscuous mode and flags attacks happening nearby. It never
//  transmits any frame. It is a detector, not a deauther.
// ============================================================================

// --- event queue filled by the RX callback, drained in tick() ---------------
struct Evt { uint8_t kind; uint8_t bssid[6]; int8_t rssi; char ssid[33]; };
// kind: 2 = beacon, 3 = probe-request
static const int QN = 128;
static Evt q[QN];                 // guarded by mux (so no volatile needed)
static uint8_t qh = 0, qt = 0;
static portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

static volatile uint32_t cDeauth = 0, cBeacon = 0;   // cumulative counters

static void extractSSID(const uint8_t* fr, int len, int off, char* out) {
  out[0] = 0;
  if (off + 2 > len) return;
  if (fr[off] != 0) return;                 // tag 0 = SSID
  int l = fr[off + 1];
  if (l > 32) l = 32;
  if (off + 2 + l > len) l = len - (off + 2);
  if (l < 0) l = 0;
  memcpy(out, fr + off + 2, l);
  out[l] = 0;
}

static void sniff(void* buf, wifi_promiscuous_pkt_type_t type) {
  const wifi_promiscuous_pkt_t* p = (const wifi_promiscuous_pkt_t*)buf;
  const uint8_t* fr = p->payload;
  int len = p->rx_ctrl.sig_len;
  uint8_t ftype = (fr[0] & 0x0C) >> 2;
  uint8_t fsub  = (fr[0] & 0xF0) >> 4;
  if (ftype != 0) return;                   // management frames only

  if (fsub == 12 || fsub == 10) { cDeauth++; return; }   // deauth / disassoc

  Evt e; e.rssi = p->rx_ctrl.rssi; e.ssid[0] = 0;
  if (fsub == 8) {                          // beacon
    cBeacon++;
    e.kind = 2; memcpy(e.bssid, fr + 16, 6);
    extractSSID(fr, len, 36, e.ssid);
  } else if (fsub == 4) {                   // probe request
    e.kind = 3; memcpy(e.bssid, fr + 10, 6);
    extractSSID(fr, len, 24, e.ssid);
    if (e.ssid[0] == 0) return;             // broadcast probe, skip
  } else {
    return;
  }

  portENTER_CRITICAL(&mux);
  uint8_t nh = (qh + 1) % QN;
  if (nh != qt) { q[qh] = e; qh = nh; }     // drop if full
  portEXIT_CRITICAL(&mux);
}

// --- aggregation state (loop side) -------------------------------------------
struct SsidRec { char ssid[33]; uint8_t bssids[4][6]; uint8_t nb; bool evil; };
static SsidRec ssids[48];
static int nSsids = 0;

struct Probe { uint8_t mac[6]; char ssid[33]; unsigned long t; };
static Probe probes[16];
static int nProbes = 0;

enum { AL_DEAUTH, AL_EVIL, AL_SPAM, AL_N };
static const char* alertMsg[AL_N] = {
  "DEAUTH / DISASSOC FLOOD detected nearby",
  "DUPLICATE SSID on multiple BSSIDs (possible EVIL TWIN)",
  "BEACON SPAM FLOOD detected nearby",
};
static unsigned long alertAt[AL_N] = {0};

static uint32_t lastDeauth = 0, lastBeacon = 0;
static int newSsidThisSec = 0;
static float deauthRate = 0, beaconRate = 0;
static int curCh = 1;
static bool probeView = false;
static unsigned long lastHop = 0, lastSec = 0, lastDraw = 0;

static int findSsid(const char* s) {
  for (int i = 0; i < nSsids; i++) if (strcmp(ssids[i].ssid, s) == 0) return i;
  return -1;
}

static void addBeacon(const char* ssid, const uint8_t* bssid) {
  if (ssid[0] == 0) return;
  int idx = findSsid(ssid);
  if (idx < 0) {
    if (nSsids >= 48) return;
    idx = nSsids++;
    strncpy(ssids[idx].ssid, ssid, 32); ssids[idx].ssid[32] = 0;
    memcpy(ssids[idx].bssids[0], bssid, 6);
    ssids[idx].nb = 1; ssids[idx].evil = false;
    newSsidThisSec++;
    return;
  }
  SsidRec& r = ssids[idx];
  for (int i = 0; i < r.nb; i++) if (memcmp(r.bssids[i], bssid, 6) == 0) return;  // known
  if (r.nb < 4) memcpy(r.bssids[r.nb++], bssid, 6);
  if (r.nb > 1) { r.evil = true; alertAt[AL_EVIL] = millis(); }
}

static void addProbe(const uint8_t* mac, const char* ssid) {
  int slot = nProbes < 16 ? nProbes++ : 0;
  if (nProbes > 16) nProbes = 16;
  if (slot == 0 && nProbes == 16) {         // ring: shift out oldest
    for (int i = 0; i < 15; i++) probes[i] = probes[i + 1];
    slot = 15;
  }
  memcpy(probes[slot].mac, mac, 6);
  strncpy(probes[slot].ssid, ssid, 32); probes[slot].ssid[32] = 0;
  probes[slot].t = millis();
}

static void drainQueue() {
  for (;;) {
    Evt e; bool got = false;
    portENTER_CRITICAL(&mux);
    if (qt != qh) { e = q[qt]; qt = (qt + 1) % QN; got = true; }
    portEXIT_CRITICAL(&mux);
    if (!got) break;
    if (e.kind == 2) addBeacon(e.ssid, e.bssid);
    else if (e.kind == 3) addProbe(e.bssid, e.ssid);
  }
}

static void resetAll() {
  portENTER_CRITICAL(&mux);
  qh = qt = 0; cDeauth = 0; cBeacon = 0;
  portEXIT_CRITICAL(&mux);
  nSsids = 0; nProbes = 0;
  lastDeauth = lastBeacon = 0; newSsidThisSec = 0;
  for (int i = 0; i < AL_N; i++) alertAt[i] = 0;
}

static void drawChrome() {
  term::clear();
  term::header("WIRELESS DEFENSE / WIDS", "defense");
  term::center(4, "detects deauth floods, evil twins & beacon spam — receive-only", pal::dim);
  term::footer("  p toggle probe-request view   r reset        Esc menu");
}

static void enter() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  esp_wifi_set_promiscuous(false);
  resetAll();
  esp_wifi_set_promiscuous_rx_cb(&sniff);
  esp_wifi_set_promiscuous(true);
  curCh = 1;
  esp_wifi_set_channel(curCh, WIFI_SECOND_CHAN_NONE);
  probeView = false;
  drawChrome();
}

static bool active(int a) { return alertAt[a] && (millis() - alertAt[a] < 5000); }

static void renderMain() {
  bool any = active(AL_DEAUTH) || active(AL_EVIL) || active(AL_SPAM);
  rgb::alert(any);

  term::box(6, 4, 7, 72, any ? pal::bad : pal::good);
  term::moveTo(7, 7);
  if (any) { term::fg(pal::bad); term::bold(true); io.print("⚠  THREATS DETECTED"); }
  else     { term::fg(pal::good); term::bold(true); io.print("✓  no attacks detected"); }
  term::reset();
  int ar = 8;
  for (int a = 0; a < AL_N; a++) {
    term::moveTo(ar, 7); io.print("\x1b[2K");
    if (active(a)) { term::fg(pal::bad); io.printf("  ▶ %s", alertMsg[a]); term::reset(); }
    ar++;
  }

  term::moveTo(14, 4); term::fg(pal::fg); term::bold(true);
  io.printf("deauth %-6lu (%.0f/s)   beacons %-6lu (%.0f/s)   SSIDs %d",
                (unsigned long)cDeauth, deauthRate, (unsigned long)cBeacon, beaconRate, nSsids);
  term::reset();

  term::moveTo(16, 4); term::fg(pal::dim); io.print("flagged SSIDs (duplicate BSSID):"); term::reset();
  int row = 17, listed = 0;
  for (int i = 0; i < nSsids && listed < 4; i++) {
    if (!ssids[i].evil) continue;
    term::moveTo(row, 6); io.print("\x1b[2K");
    term::fg(pal::warn);
    io.printf("⚑ %-24s  seen on %d BSSIDs", ssids[i].ssid, ssids[i].nb);
    term::reset();
    row++; listed++;
  }
  for (; row < 21; row++) { term::moveTo(row, 6); io.print("\x1b[2K"); }
}

static void renderProbes() {
  rgb::alert(false);
  term::moveTo(6, 4); term::fg(pal::purple); term::bold(true);
  io.print("PROBE REQUESTS — networks nearby devices are searching for");
  term::reset();
  term::moveTo(7, 4); term::fg(pal::dim);
  io.print("(phones leak their saved-network history in plain probe requests)");
  term::reset();
  for (int i = 0; i < 14; i++) {
    int row = 9 + i; term::moveTo(row, 4); io.print("\x1b[2K");
    if (i < nProbes) {
      const Probe& p = probes[nProbes - 1 - i];
      term::fg(pal::fg);
      io.printf("%02x:%02x:%02x:%02x:%02x:%02x  →  ",
                    p.mac[0], p.mac[1], p.mac[2], p.mac[3], p.mac[4], p.mac[5]);
      term::fg(pal::purple); io.print(p.ssid);
      term::reset();
    }
  }
}

static void tick() {
  unsigned long now = millis();
  drainQueue();

  if (now - lastHop > 280) {
    curCh = (curCh % 13) + 1;
    esp_wifi_set_channel(curCh, WIFI_SECOND_CHAN_NONE);
    lastHop = now;
  }
  if (now - lastSec >= 1000) {
    uint32_t d = cDeauth, b = cBeacon;
    deauthRate = (float)(d - lastDeauth);
    beaconRate = (float)(b - lastBeacon);
    lastDeauth = d; lastBeacon = b;
    if (deauthRate >= 10)                      { alertAt[AL_DEAUTH] = now; rgb::flash(pal::bad); }
    if (beaconRate >= 120 || newSsidThisSec >= 20) alertAt[AL_SPAM] = now;
    newSsidThisSec = 0;
    lastSec = now;
  }
  if (now - lastDraw > 200) {
    if (probeView) renderProbes(); else renderMain();
    lastDraw = now;
  }
}

static void leave() {
  esp_wifi_set_promiscuous(false);
  esp_wifi_set_promiscuous_rx_cb(NULL);
  WiFi.mode(WIFI_OFF);
  rgb::alert(false);
}

static void onKey(int k) {
  if (k == 'p' || k == 'P') { probeView = !probeView; drawChrome(); lastDraw = 0; }
  else if (k == 'r' || k == 'R') resetAll();
}

static const Cmd cmds[] = {
  { "probe", 'p', "toggle probe-request sniffer" },
  { "reset", 'r', "reset counters" },
};

const Screen screenWIDS = { "Wireless Defense (WIDS)", "defense", enter, tick, leave, onKey, SCMDS(cmds), nullptr };
