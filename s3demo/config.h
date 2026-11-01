#pragma once
#include <Arduino.h>

// ============================================================================
//  s3demo — all-in-one ESP32-S3 demo firmware
//  Build target: bare ESP32-S3-DevKitC-1 / generic, no soldered pins.
// ============================================================================

#define S3DEMO_VERSION   "1.0"
#define SERIAL_BAUD      115200

// ---- Onboard RGB LED (WS2812) ------------------------------------------------
// DevKitC-1 v1.0 and most clones drive the onboard LED from GPIO48.
// DevKitC-1 v1.1 moved it to GPIO38.
// This is only the first-boot default; the pin is overridable at runtime with
// the `pin <n>` command and persisted to NVS (see config.cpp / Preferences).
#define DEFAULT_LED_PIN  48
#define LED_COUNT        1        // bare board = one pixel; raise if you plug in a strip

// ---- Physical input ----------------------------------------------------------
#define BOOT_BUTTON_PIN  0        // BOOT button — usable with no soldering

// ---- USB-HID demo ------------------------------------------------------------
// Benign preset typed by the HID demo. Edit freely, but keep it harmless.
#define HID_PRESET_TEXT  "Hello from an ESP32-S3 :) no keyboard attached!\n"

// ---- Runtime config persisted to NVS ----------------------------------------
namespace cfg {
  void     begin();                 // load persisted settings from NVS
  uint8_t  ledPin();                // current LED data pin
  void     setLedPin(uint8_t pin);  // set + persist LED data pin
}
