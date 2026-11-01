#pragma once
#include <Arduino.h>

// Nordic UART Service (NUS) BLE peripheral — lets a phone BLE-serial app drive
// the console. Owns the NimBLE stack (init once at boot); the BLE scanner reuses
// it without init/deinit.
namespace bleserial {
  void begin();                                 // init NimBLE + NUS + advertise "littleHAKR"
  bool connected();
  void send(const uint8_t* buf, size_t len);    // chunked NOTIFY with light backpressure
}
