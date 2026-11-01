#include "io.h"
#include "bleserial.h"

IoStream io;

// ---------------------------------------------------------------------------
//  BLE input ring (filled from the NimBLE RX callback, drained in loop())
// ---------------------------------------------------------------------------
static portMUX_TYPE rxMux = portMUX_INITIALIZER_UNLOCKED;
static const int    RXN   = 256;
static uint8_t      rxBuf[RXN];
static volatile int rxHead = 0, rxTail = 0;

void ioBleRxPush(const uint8_t* data, size_t len) {
  portENTER_CRITICAL(&rxMux);
  for (size_t i = 0; i < len; i++) {
    int nxt = (rxHead + 1) % RXN;
    if (nxt == rxTail) break;   // full — drop rest
    rxBuf[rxHead] = data[i];
    rxHead = nxt;
  }
  portEXIT_CRITICAL(&rxMux);
}

static int rxCount() {
  portENTER_CRITICAL(&rxMux);
  int n = (rxHead - rxTail + RXN) % RXN;
  portEXIT_CRITICAL(&rxMux);
  return n;
}
static int rxPop() {
  int b = -1;
  portENTER_CRITICAL(&rxMux);
  if (rxHead != rxTail) { b = rxBuf[rxTail]; rxTail = (rxTail + 1) % RXN; }
  portEXIT_CRITICAL(&rxMux);
  return b;
}
static int rxPeek() {
  int b = -1;
  portENTER_CRITICAL(&rxMux);
  if (rxHead != rxTail) b = rxBuf[rxTail];
  portEXIT_CRITICAL(&rxMux);
  return b;
}

// ---------------------------------------------------------------------------
//  ANSI -> text grid (only the subset term.cpp emits)
// ---------------------------------------------------------------------------
static const int GW = 80, GH = 24;
static char grid[GH][GW + 1];
static int  cr = 0, cc = 0;            // cursor (row, col), 0-based
static bool gridDirty = false;
static unsigned long lastFlush = 0;
static unsigned long keyAt     = 0;    // last time input arrived (prompt flush)

// ESC-sequence parser state
enum { S_NORMAL, S_ESC, S_CSI };
static int  pstate = S_NORMAL;
static char pbuf[24];
static int  plen = 0;

// UTF-8 multibyte accumulation
static uint32_t ucp = 0;     // code point being assembled
static int      ucont = 0;   // continuation bytes remaining

static void gridClear() {
  for (int r = 0; r < GH; r++) { memset(grid[r], ' ', GW); grid[r][GW] = 0; }
  cr = cc = 0;
}

// Map the glyphs term.cpp uses to a single ASCII cell so phone text stays aligned.
static char mapGlyph(uint32_t cp) {
  switch (cp) {
    case 0x2500: case 0x2501: return '-';   // ─ ━
    case 0x2502: case 0x2503: return '|';   // │ ┃
    case 0x250C: case 0x2510: case 0x2514: case 0x2518:
    case 0x251C: case 0x2524: case 0x252C: case 0x2534: case 0x253C:
      return '+';                           // box corners/tees
    case 0x2588: case 0x2593: case 0x2592: return '#'; // █ ▓ ▒
    case 0x2591: return '.';                // ░
    case 0x00B7: case 0x2022: case 0x25CF: return '*'; // · • ●
    case 0x2191: return '^';                // ↑
    case 0x2193: return 'v';                // ↓
    case 0x2192: case 0x25B8: case 0x25B6: return '>'; // → ▸ ▶
    case 0x2190: return '<';                // ←
    case 0x2713: case 0x2714: return '+';   // ✓ ✔
    case 0x2717: case 0x2718: return 'x';   // ✗ ✘
    case 0x26A0: case 0x2691: return '!';   // ⚠ ⚑
    case 0x26A1: return '*';                // ⚡
    case 0x2728: return '*';                // ✨
    default:     return (cp < 128) ? (char)cp : '?';
  }
}

static void putCell(char c) {
  if (cr < 0) cr = 0; if (cr >= GH) cr = GH - 1;
  if (cc >= 0 && cc < GW) { grid[cr][cc] = c; }
  if (cc < GW) cc++;
  gridDirty = true;
}

static void feedGrid(uint8_t b) {
  // mid UTF-8 sequence?
  if (ucont > 0) {
    if ((b & 0xC0) == 0x80) {                // valid continuation byte
      ucp = (ucp << 6) | (b & 0x3F);
      if (--ucont == 0) putCell(mapGlyph(ucp));
      return;
    }
    ucont = 0;                               // malformed — resync, re-handle b below
  }

  switch (pstate) {
    case S_NORMAL:
      if (b == 0x1B) { pstate = S_ESC; plen = 0; return; }
      if (b == '\r') { cc = 0; return; }
      if (b == '\n') { cr++; if (cr >= GH) cr = GH - 1; return; }
      if (b == '\t') { cc = (cc + 8) & ~7; if (cc >= GW) cc = GW - 1; return; }
      if (b >= 0x20 && b < 0x7F) { putCell((char)b); return; }
      if (b >= 0xC0) {                       // UTF-8 lead byte
        if ((b & 0xE0) == 0xC0) { ucp = b & 0x1F; ucont = 1; }
        else if ((b & 0xF0) == 0xE0) { ucp = b & 0x0F; ucont = 2; }
        else if ((b & 0xF8) == 0xF0) { ucp = b & 0x07; ucont = 3; }
        return;
      }
      return;                                // ignore other control bytes

    case S_ESC:
      if (b == '[') { pstate = S_CSI; plen = 0; }
      else          { pstate = S_NORMAL; }   // ignore 2-char escapes
      return;

    case S_CSI:
      if (b >= 0x40 && b <= 0x7E) {          // final byte
        pbuf[plen] = 0;
        // parse up to two numeric params
        int p0 = -1, p1 = -1;
        {
          const char* s = pbuf;
          if (*s == '?') s++;                // private-mode marker
          if (*s >= '0' && *s <= '9') { p0 = atoi(s); while (*s >= '0' && *s <= '9') s++; }
          if (*s == ';') { s++; if (*s >= '0' && *s <= '9') p1 = atoi(s); }
        }
        switch (b) {
          case 'H': case 'f':                // cursor position (1-based)
            cr = (p0 < 1 ? 1 : p0) - 1;
            cc = (p1 < 1 ? 1 : p1) - 1;
            if (cr >= GH) cr = GH - 1; if (cc >= GW) cc = GW - 1;
            break;
          case 'J':                          // erase display (2 = all)
            if (p0 == 2 || p0 < 0) { gridClear(); gridDirty = true; }
            break;
          case 'K': {                        // erase line
            int from = (p0 == 2 || p0 == 1) ? 0 : cc;
            int to   = (p0 == 1) ? cc : GW;
            for (int i = from; i < to && i < GW; i++) grid[cr][i] = ' ';
            gridDirty = true;
            break;
          }
          // 'm' (SGR colour), 'h'/'l' (cursor visibility), others: ignore
          default: break;
        }
        pstate = S_NORMAL;
      } else if (plen < (int)sizeof(pbuf) - 1) {
        pbuf[plen++] = (char)b;
      }
      return;
  }
}

// ---------------------------------------------------------------------------
//  Stream interface
// ---------------------------------------------------------------------------
void IoStream::begin() { gridClear(); }

size_t IoStream::write(uint8_t b) {
  Serial.write(b);                           // USB CDC: raw ANSI, always
  if (bleserial::connected()) feedGrid(b);   // BLE: build the text grid
  return 1;
}

size_t IoStream::write(const uint8_t* buf, size_t len) {
  Serial.write(buf, len);
  if (bleserial::connected()) for (size_t i = 0; i < len; i++) feedGrid(buf[i]);
  return len;
}

// read()/available()/peek() serve the interactive USB terminal only. BLE input is
// line-buffered separately (bleNextLine) so typed phone commands don't get consumed
// as single keypresses by term::poll().
int IoStream::available() { return Serial.available(); }
int IoStream::read()      { return Serial.read(); }
int IoStream::peek()      { return Serial.peek(); }

void IoStream::markActivity() { keyAt = millis(); }

// Assemble a line from the BLE RX ring. Handles LF, CR, and CRLF terminators;
// backspace edits; ignores other control bytes. Returns true once a line is ready
// (out is NUL-terminated, trailing \r/\n stripped). An empty line returns true too
// (caller treats it as a redraw/refresh).
bool IoStream::bleNextLine(char* out, size_t cap) {
  static char line[96];
  static size_t len = 0;
  static uint8_t prev = 0;
  while (rxCount() > 0) {
    int ci = rxPop();
    if (ci < 0) break;
    uint8_t c = (uint8_t)ci;
    keyAt = millis();
    if (c == '\n' && prev == '\r') { prev = c; continue; }   // swallow LF of CRLF
    prev = c;
    if (c == '\n' || c == '\r') {
      size_t n = len < cap - 1 ? len : cap - 1;
      memcpy(out, line, n); out[n] = 0;
      len = 0;
      return true;
    }
    if (c == 0x08 || c == 0x7F) { if (len) len--; continue; }  // backspace
    if (c >= 0x20 && c < 0x7F && len < sizeof(line) - 1) line[len++] = (char)c;
  }
  return false;
}

static const int SNAPCAP = GH * (GW + 2) + 8;
static char snapBuf[SNAPCAP];
static char lastBuf[SNAPCAP];
static int  snapLen = 0, lastLen = 0;
static bool bleAnimated = false;

void IoStream::setBleAnimated(bool on) { bleAnimated = on; }

void IoStream::forceFlush() { gridDirty = true; lastLen = 0; keyAt = millis(); }

void IoStream::onBleConnect() {
  gridDirty = true;
  lastFlush = 0;                             // force an immediate flush
  lastLen   = 0;                             // defeat dedupe for the first frame
  keyAt     = millis();                      // open a flush window even on anim screens
}

// Serialize the grid to trimmed plain text into snapBuf.
static void buildSnapshot() {
  int n = 0;
  int lastRow = GH - 1;
  while (lastRow > 0) {                       // drop trailing blank rows
    bool blank = true;
    for (int c = 0; c < GW; c++) if (grid[lastRow][c] != ' ') { blank = false; break; }
    if (!blank) break;
    lastRow--;
  }
  for (int r = 0; r <= lastRow; r++) {
    int end = GW;
    while (end > 0 && grid[r][end - 1] == ' ') end--;   // trim trailing spaces
    for (int c = 0; c < end && n < SNAPCAP - 3; c++) snapBuf[n++] = grid[r][c];
    if (n < SNAPCAP - 2) { snapBuf[n++] = '\r'; snapBuf[n++] = '\n'; }
  }
  snapLen = n;
}

void IoStream::service() {
  if (!bleserial::connected() || !gridDirty) return;
  unsigned long now = millis();
  bool recentKey = (keyAt && now - keyAt < 1200);
  // Live/animated screens: only push right after a keypress (or connect), never on
  // idle, so continuous animation doesn't flood the phone. Stay dirty until then.
  if (bleAnimated && !recentKey) return;

  unsigned long interval = recentKey ? 150 : 800;
  if (now - lastFlush < interval) return;

  buildSnapshot();
  if (snapLen == lastLen && memcmp(snapBuf, lastBuf, snapLen) == 0) {
    gridDirty = false; return;                // unchanged — don't resend
  }
  bleserial::send((const uint8_t*)snapBuf, snapLen);
  memcpy(lastBuf, snapBuf, snapLen); lastLen = snapLen;
  gridDirty = false; lastFlush = now;
}
