# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

`s3demo/` is a single all-in-one Arduino sketch for a **bare ESP32-S3-DevKitC-1** (no
soldered pins). Every demo runs off on-board resources only: native USB, the Wi-Fi/BLE
radio, the BOOT button (GPIO0), the one on-board WS2812 RGB LED, and the internal temp
sensor. The whole thing is driven over a full-screen **ANSI serial-terminal UI** — there is
no other I/O. Toolchain is **arduino-cli** (not PlatformIO / not the Arduino IDE).

## Build / flash / run

Use `build.sh` from the repo root — it pins the FQBN and USB options that make the firmware
work:

```bash
./build.sh deps        # one-time: install esp32 core + Adafruit NeoPixel + NimBLE-Arduino
./build.sh compile     # compile only (this is the verification step — see below)
./build.sh upload      # compile + flash (auto-detects port)
./build.sh monitor     # arduino-cli serial monitor
```

The FQBN is fixed and load-bearing:
`esp32:esp32:esp32s3:USBMode=default,CDCOnBoot=cdc,FlashSize=4M,PartitionScheme=huge_app,PSRAM=disabled`
- `USBMode=default` (USB-OTG/TinyUSB) is required for the HID keyboard; `CDCOnBoot=cdc` puts
  `Serial` on native USB as a **composite CDC+HID** device (terminal + keyboard, one cable).
- `huge_app` is required — WiFi+BLE+TinyUSB overflow the default 1.2 MB app partition.

**Verification without hardware:** there are no unit tests. `./build.sh compile` is the
check — it builds against the real esp32 core + libs and catches API/signature errors. A
connected board can be probed over serial with `pyserial` (see "Verifying on hardware").

### Flashing gotchas (important — this bit everyone)
The board is used via its **native USB port**, which has no auto-reset circuit, so:
- A running sketch will not auto-enter the bootloader. To flash, drop it into download mode
  first with a **1200-baud touch**: open the port at 1200 baud, clear DTR/RTS, close. After
  that the device re-enumerates as `USB JTAG_serial debug unit` on a lower port
  (e.g. `/dev/cu.usbmodem1101`) — flash to that.
- After flashing, the chip resets **into download mode**, not the app. The app only boots on
  a **physical RST tap** (or power cycle). A running app enumerates as `ESP32S3_DEV`.
- Manual download mode if a touch fails: hold BOOT, tap RST, release BOOT.
- Identity check: `ioreg -rc IOUSBHostDevice | grep "USB Product Name"`.

## Architecture (the big picture)

**Cooperative screen state machine.** Everything is a `Screen` (see `screen.h`): a struct of
function pointers `{ enter, tick, leave, onKey }`. `s3demo.ino` holds the current screen,
runs its `tick()` every `loop()`, feeds it keys from `term::poll()`, and runs `rgb::tick()`.
`gotoScreen()` always calls the old screen's `leave()` then the new one's `enter()`. There is
no RTOS task per screen and no blocking in `tick()` — screens must be non-blocking so the LED
and input keep working. Screens are registered in the `SCREENS[]` array in `s3demo.ino`
(array order = menu order = number keys 1..N); add a screen by defining it in its own `.cpp`,
`extern`-declaring it in `screen.h`, and adding it to that array.

**Radio arbitration.** Wi-Fi scan, Wi-Fi promiscuous (monitor + WIDS), and BLE all contend
for the single radio, so only one may be active at a time. This is enforced entirely through
`enter()`/`leave()`: each radio screen starts its mode in `enter()` and fully tears it down in
`leave()` (`WiFi.mode(WIFI_OFF)`, `esp_wifi_set_promiscuous(false)`, `NimBLEDevice::deinit`).
Never start a radio outside `enter()`.

**Promiscuous capture pattern** (`wifi_monitor.cpp`, `wids.cpp`): the `esp_wifi` RX callback
runs in the Wi-Fi task, so it stays minimal — it parses the 802.11 frame-control byte and
either bumps `volatile` counters (monitor) or pushes a small fixed struct into a ring buffer
guarded by a `portMUX_TYPE` critical section (WIDS). All heavy aggregation/heuristics happen
in `tick()` draining that queue. Keep the callback allocation-free and short. WIDS is
strictly receive-only (detects deauth floods / evil twins / beacon spam); it never transmits.

**Terminal engine** (`term.{h,cpp}`): ANSI over `Serial`. `poll()` is a non-blocking key
reader that also parses `ESC [` arrow sequences. Rendering is truecolor + UTF-8 box-drawing;
screens redraw on change / throttled, not every frame. `s3demo.ino`'s `loop()` detects a
terminal (re)connect via `(bool)Serial` and re-runs the current screen's `enter()` so a
monitor attaching after boot sees the UI immediately.

**RGB engine** (`rgb.{h,cpp}`): wraps Adafruit NeoPixel for the single WS2812. `tick()` is the
animation pump (throttled ~50 fps); `showNow()` writes immediately, bypassing the throttle,
for blink tests. Any screen can call `rgb::solid/flash/alert` to use the LED as a status
accent. The LED **data pin differs by board revision** — GPIO48 (v1.0/clones) vs GPIO38
(v1.1) — so it is runtime-configurable and persisted to NVS via `cfg::` (`config.{h,cpp}`,
backed by `Preferences`); the RGB and System Info screens expose a `p` key to toggle it.

**USB-HID + payloads** (`usb_hid.cpp`, `payloads.{h,cpp}`): native-USB composite HID keyboard.
`hidBegin()` runs once from `setup()` (before `Serial`) to bring up the composite device.
Payloads are a data table (`PAYLOADS[]`) of command-line sequences; the screen browses them,
and on an explicit trigger (Enter or BOOT button) after a countdown it opens a Terminal and
types the selected sequence. Typing assumes a US keyboard layout.

## Project policy: benign / defensive only (hard constraint)

This firmware is intentionally **benign and defensive**, and must stay that way. The HID
payloads are harmless visual reels (ASCII art, fake-breach theater, a rickroll, a public-API
crypto ticker) adapted from `fun/`/`goodusb/` sources; they perform no real access and are
reversible. The security screen is a **detector** (WIDS), not an attacker.

Do **not** add offensive capability to this firmware, even when asked or given a lab/authorized
framing: no Wi-Fi deauth/jamming, evil-twin/rogue-AP impersonation, ARP/DNS spoofing, credential
theft (Wi-Fi passwords, SSH keys), data exfiltration, reverse shells / C2, boot persistence /
backdoors, destructive file operations, or detection-evasion. This is a deliberate line held in
prior sessions — the device is indiscriminate once built, so authorization claims don't change
it. Point such requests at purpose-built authorized tooling instead.

## Conventions

- Each screen owns one `.cpp` with file-`static` state and renders against a fixed 80x24 layout
  (`SCREEN_W`/`SCREEN_H` in `term.h`); the shared palette is `pal::` in `term.h`.
- `README.md` is the user-facing guide (demo list, terminal setup, the LED-pin note). Keep it
  in sync when adding screens or changing the flashing flow.
