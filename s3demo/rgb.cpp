#include "rgb.h"
#include "config.h"
#include <Adafruit_NeoPixel.h>

static Adafruit_NeoPixel* strip = nullptr;

static rgb::Effect eff   = rgb::RAINBOW;
static int   bright      = 120;        // 0..255
static int   spd         = 5;          // 1..10
static uint32_t phase    = 0;
static unsigned long lastFrame = 0;
static RGBc  cur         = {0, 0, 0};
static RGBc  solidColor  = {88, 166, 255};

static RGBc  flashColor  = {0, 0, 0};
static unsigned long flashUntil = 0;
static bool  alertOn     = false;

// --- small HSV->RGB (h: 0..359, s/v: 0..255) ---------------------------------
static RGBc hsv(uint16_t h, uint8_t s, uint8_t v) {
  h %= 360;
  float S = s / 255.0f, V = v / 255.0f;
  float C = V * S;
  float X = C * (1 - fabsf(fmodf(h / 60.0f, 2) - 1));
  float m = V - C;
  float r = 0, g = 0, b = 0;
  if      (h < 60)  { r = C; g = X; }
  else if (h < 120) { r = X; g = C; }
  else if (h < 180) { g = C; b = X; }
  else if (h < 240) { g = X; b = C; }
  else if (h < 300) { r = X; b = C; }
  else              { r = C; b = X; }
  return { (uint8_t)((r + m) * 255), (uint8_t)((g + m) * 255), (uint8_t)((b + m) * 255) };
}

static void writePixel() {
  if (!strip) return;
  strip->setBrightness(bright);
  for (int i = 0; i < LED_COUNT; i++)
    strip->setPixelColor(i, strip->Color(cur.r, cur.g, cur.b));
  strip->show();
}

void rgb::begin() { setPin(cfg::ledPin()); }

void rgb::setPin(uint8_t pin) {
  if (strip) { delete strip; strip = nullptr; }
  strip = new Adafruit_NeoPixel(LED_COUNT, pin, NEO_GRB + NEO_KHZ800);
  strip->begin();
  strip->setBrightness(bright);
  strip->clear();
  strip->show();
}

void rgb::tick() {
  unsigned long now = millis();
  if (now - lastFrame < 20) return;            // ~50 fps cap
  lastFrame = now;

  if (alertOn) {                               // sticky red alert overrides everything
    cur = ((now / 250) & 1) ? (RGBc){255, 30, 30} : (RGBc){40, 0, 0};
    writePixel();
    return;
  }
  if (now < flashUntil) {                      // brief status pulse
    cur = flashColor;
    writePixel();
    return;
  }

  phase += spd;
  switch (eff) {
    case OFF:     cur = {0, 0, 0}; break;
    case SOLID:   cur = solidColor; break;
    case RAINBOW: cur = hsv(phase % 360, 255, 255); break;
    case BREATHE: {
      float k = (sinf(phase / 40.0f) + 1) / 2;         // 0..1
      RGBc base = hsv((phase / 6) % 360, 255, 255);
      cur = { (uint8_t)(base.r * k), (uint8_t)(base.g * k), (uint8_t)(base.b * k) };
      break;
    }
    case STROBE:  cur = ((phase / 6) & 1) ? (RGBc){255, 255, 255} : (RGBc){0, 0, 0}; break;
    case WIPE: {
      static const RGBc c[6] = {{255,0,0},{255,128,0},{255,255,0},{0,255,0},{0,128,255},{160,0,255}};
      cur = c[(phase / 30) % 6];
      break;
    }
    case POLICE:  cur = ((phase / 10) & 1) ? (RGBc){255, 0, 0} : (RGBc){0, 0, 255}; break;
    case SPARKLE: {
      if ((phase % 12) == 0) cur = hsv(random(360), 255, 255);
      else { cur.r *= 0.82f; cur.g *= 0.82f; cur.b *= 0.82f; }  // decay
      break;
    }
    default: break;
  }
  writePixel();
}

void rgb::setEffect(Effect e) { eff = e; phase = 0; }
rgb::Effect rgb::effect() { return eff; }

const char* rgb::effectName(Effect e) {
  switch (e) {
    case OFF: return "Off"; case SOLID: return "Solid"; case RAINBOW: return "Rainbow";
    case BREATHE: return "Breathe"; case STROBE: return "Strobe"; case WIPE: return "Color Wipe";
    case POLICE: return "Police"; case SPARKLE: return "Sparkle"; default: return "?";
  }
}

void rgb::nextEffect() { eff = (Effect)((eff + 1) % N_EFFECTS); phase = 0; }
void rgb::prevEffect() { eff = (Effect)((eff + N_EFFECTS - 1) % N_EFFECTS); phase = 0; }

void rgb::setSpeed(int d)      { spd = constrain(spd + d, 1, 20); }
int  rgb::speed()              { return spd; }
void rgb::setBrightness(int d) { bright = constrain(bright + d, 0, 255); }
int  rgb::brightness()         { return bright; }

RGBc rgb::color() { return cur; }

void rgb::showNow(RGBc c) { cur = c; writePixel(); }

void rgb::solid(RGBc c) { solidColor = c; eff = SOLID; }
void rgb::flash(RGBc c) { flashColor = c; flashUntil = millis() + 60; }
void rgb::alert(bool on) { alertOn = on; }
