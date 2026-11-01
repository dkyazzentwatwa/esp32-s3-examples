#pragma once
#include <Arduino.h>

// A typed command for the phone/BLE console (see console.cpp). `word` is what the
// user types; if `key` != 0 the console feeds that key to the screen's onKey(),
// otherwise the screen's onCmd() handles it. `help` is the one-line description.
struct Cmd {
  const char* word;
  int         key;
  const char* help;
};

// A screen is a cooperative, non-blocking state module. The dispatcher in
// s3demo.ino calls tick() every loop and feeds keys to onKey(). enter()/leave()
// set up and tear down resources (radios, etc.) so demos never fight over the chip.
// The last three fields are optional (zero/null) — they add a typed command
// interface for phone BLE apps that can't send arrow keys or single keypresses.
struct Screen {
  const char* title;                 // shown in the header
  const char* tag;                   // short category tag, e.g. "visual"
  void (*enter)();
  void (*tick)();
  void (*leave)();
  void (*onKey)(int key);
  const Cmd*  cmds;                   // optional: typed commands (for `help` + exec)
  int         nCmds;
  bool (*onCmd)(const char* line);   // optional: custom typed-command handler
};

// sizeof helper for a screen's Cmd table
#define SCMDS(a) (a), (int)(sizeof(a) / sizeof((a)[0]))

// Navigation (implemented in s3demo.ino)
void gotoScreen(const Screen* s);
void gotoMenu();
const Screen* currentScreen();

// Registry (defined in s3demo.ino)
extern const Screen* const SCREENS[];
extern const int SCREEN_COUNT;

// Screen instances (each defined in its own .cpp)
extern const Screen screenMenu;
extern const Screen screenRGB;
extern const Screen screenWifiScan;
extern const Screen screenWifiMon;
extern const Screen screenBLE;
extern const Screen screenWIDS;
extern const Screen screenHID;
extern const Screen screenSysInfo;

// USB-HID lifecycle (defined in usb_hid.cpp, called once from setup)
void hidBegin();
