#pragma once
#include <Arduino.h>

// Transport shim. The UI emits ANSI to `io` instead of `Serial`:
//  - USB branch: raw passthrough to USB CDC (full ANSI, unchanged).
//  - BLE branch (while a phone is connected): the same ANSI stream is parsed
//    into an in-RAM 24x80 text grid and flushed as compact plain-text snapshots
//    over the Nordic UART Service, so basic phone apps show readable text.
// Input from USB and BLE is merged so term::poll() works over either link.
class IoStream : public Stream {
public:
  size_t write(uint8_t b) override;
  size_t write(const uint8_t* buf, size_t len) override;
  int    available() override;
  int    read() override;
  int    peek() override;
  void   flush() override {}
  using Print::write;

  void begin();
  void service();          // call every loop(): rate-limited BLE snapshot flush
  void onBleConnect();     // force a fresh snapshot shortly after a phone connects
  void setBleAnimated(bool on);  // live screens: push to BLE only after a keypress,
                                 // so continuous animation doesn't spam the phone

  // Phone console input: BLE bytes are line-buffered (not merged into read()),
  // because phones type whole commands instead of single keys / arrows.
  bool bleNextLine(char* out, size_t cap);   // true when a full line is ready
  void markActivity();     // open a short BLE flush window (after a command redraw)
  void forceFlush();       // re-send the current screen snapshot to BLE now
};

extern IoStream io;

// Called by the BLE RX characteristic callback (NimBLE task context).
void ioBleRxPush(const uint8_t* data, size_t len);
