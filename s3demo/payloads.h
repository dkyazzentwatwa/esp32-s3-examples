#pragma once

// Benign HID payloads for the USB-HID screen. These are adapted from the
// "fun" / "goodusb" creator reels in cypher-drive/payloads: visual theater
// (ASCII rain, fake-breach, a rickroll, a public-API crypto ticker, app pranks).
// Each is reversible and performs NO real access, credential theft, exfil,
// persistence, or network attack. Offensive payloads are intentionally excluded.
struct Payload {
  const char*        title;
  const char*        vibe;          // one-line description shown in the UI
  bool               opensTerminal; // run a Spotlight -> Terminal open first (macOS)
  const char* const* cmds;          // each line is typed, then Enter
  int                nCmds;
};

extern const Payload PAYLOADS[];
extern const int PAYLOAD_COUNT;
