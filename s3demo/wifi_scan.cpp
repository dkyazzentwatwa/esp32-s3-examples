#include "screen.h"
#include "term.h"
#include "rgb.h"
#include <WiFi.h>

static bool scanning = false;
static unsigned long lastKick = 0;

static const char* encStr(wifi_auth_mode_t m) {
  switch (m) {
    case WIFI_AUTH_OPEN:            return "OPEN";
    case WIFI_AUTH_WEP:             return "WEP";
    case WIFI_AUTH_WPA_PSK:         return "WPA";
    case WIFI_AUTH_WPA2_PSK:        return "WPA2";
    case WIFI_AUTH_WPA_WPA2_PSK:    return "WPA/2";
    case WIFI_AUTH_WPA3_PSK:        return "WPA3";
    case WIFI_AUTH_WPA2_WPA3_PSK:   return "WPA2/3";
    case WIFI_AUTH_WPA2_ENTERPRISE: return "ENT";
    default:                        return "?";
  }
}
static RGBc rssiColor(int r) {
  if (r >= -55) return pal::good;
  if (r >= -70) return pal::warn;
  return pal::bad;
}

static void kick() {
  WiFi.scanDelete();
  WiFi.scanNetworks(true, false);   // async, hide hidden
  scanning = true;
  lastKick = millis();
}

static void enter() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(50);
  term::clear();
  term::header("WI-FI SCANNER", "recon");
  term::moveTo(4, 4); term::fg(pal::dim); term::bold(true);
  io.printf("%-24s %-4s %-4s %-18s %s", "SSID", "CH", "dBm", "SIGNAL", "ENC");
  term::reset();
  term::footer("  live scan · strongest first        r rescan    Esc menu");
  kick();
}

static void render(int n) {
  int idx[40];
  int m = n > 40 ? 40 : n;
  for (int i = 0; i < m; i++) idx[i] = i;
  for (int i = 0; i < m; i++)                              // simple sort by RSSI desc
    for (int j = i + 1; j < m; j++)
      if (WiFi.RSSI(idx[j]) > WiFi.RSSI(idx[i])) { int t = idx[i]; idx[i] = idx[j]; idx[j] = t; }

  int shown = m > 16 ? 16 : m;
  for (int i = 0; i < shown; i++) {
    int e = idx[i], row = 5 + i, rssi = WiFi.RSSI(e);
    String ssid = WiFi.SSID(e);
    if (ssid.length() == 0) ssid = "<hidden>";
    if (ssid.length() > 22) ssid = ssid.substring(0, 22);
    term::clearLine(row);
    term::moveTo(row, 4); term::fg(pal::fg);
    io.printf("%-24s %-4d %-4d ", ssid.c_str(), WiFi.channel(e), rssi);
    term::reset();
    term::bar(row, 39, 16, (rssi + 100) / 60.0f, rssiColor(rssi));
    term::moveTo(row, 58); term::fg(pal::dim);
    io.printf("%s", encStr(WiFi.encryptionType(e)));
    term::reset();
  }
  for (int i = shown; i < 16; i++) term::clearLine(5 + i);

  term::moveTo(22, 4); term::fg(pal::accent); term::bold(true);
  io.printf("%d networks found", n);
  term::reset();
  if (n > 0) rgb::solid(rssiColor(WiFi.RSSI(idx[0])));
}

static void tick() {
  if (scanning) {
    int n = WiFi.scanComplete();
    if (n >= 0) { scanning = false; render(n); }
  } else if (millis() - lastKick > 4000) {
    kick();
  }
}

static void leave() {
  WiFi.scanDelete();
  WiFi.mode(WIFI_OFF);
}

static void onKey(int k) { if (k == 'r' || k == 'R') kick(); }

static const Cmd cmds[] = { { "rescan", 'r', "scan for networks again" } };

const Screen screenWifiScan = { "Wi-Fi Scanner", "recon", enter, tick, leave, onKey, SCMDS(cmds), nullptr };
