# s3demo — all-in-one ESP32-S3 demo firmware

A single firmware for a **bare ESP32-S3-DevKitC-1** (no soldered pins, nothing wired up),
driven entirely over a slick **ANSI serial-terminal UI**. Built for quick, filmable demos.

Everything runs off what's already on the board: native USB, the Wi-Fi/BLE radio, the BOOT
button, the onboard WS2812 RGB LED, and the internal temperature sensor.

> **The visual is the terminal.** A bare board has exactly one RGB LED, so the full-screen
> color terminal UI (plus the single LED as an accent) is what reads on camera.

## Demos

| # | Demo | What it shows |
|---|------|---------------|
| 1 | **RGB Light Show** (visual) | WS2812 effects — rainbow, breathe, strobe, wipe, police, sparkle — mirrored as a big on-screen color band |
| 2 | **Wi-Fi Scanner** (recon) | Live network table with colored signal bars, channel, encryption |
| 3 | **Wi-Fi Packet Monitor** (sniff) | Passive promiscuous capture, per-channel frame bar chart + packets/sec. **Receive-only.** |
| 4 | **BLE Scanner** (recon) | Nearby phones/watches/earbuds with live RSSI bars |
| 5 | **Wireless Defense / WIDS** (defense) | **Detects** deauth floods, evil twins, beacon spam as they happen + a passive probe-request privacy view. Receive-only, never transmits. |
| 6 | **USB-HID Demo** (inject) | The board enumerates as a USB keyboard and types a benign preset after a visible countdown |
| 7 | **System Info** (stats) | Chip, cores, flash/PSRAM, internal temp, heap, MAC, uptime, LED pin |

### Scope / ethics
This firmware is **defensive and benign by design**. It does **not** deauth/jam, impersonate
APs, harvest credentials, spam beacons, or capture handshakes — it never transmits attack
frames. The WIDS screen *detects* those attacks; the HID demo only types a harmless string as
a self-demo. Only run the security screens on networks/devices you own or are authorized to test.

## Build & flash (arduino-cli)

Prereqs: `arduino-cli`, the `esp32:esp32` core, and the `Adafruit NeoPixel` + `NimBLE-Arduino`
libraries. Install them all with:

```bash
./build.sh deps
```

Compile:

```bash
./build.sh compile
```

Flash **via the UART port** (the one through the CP210x/CH343 bridge — most reliable):

```bash
./build.sh upload            # auto-detects the port, or: ./build.sh upload /dev/cu.xxx
```

### The two USB ports (important)
The DevKitC-1 has two USB-C ports:
- **UART port** (bridge chip) → use this to **flash**.
- **USB port** (native USB) → the firmware's serial terminal **and** the HID keyboard run here.

The build sets `USB CDC On Boot = Enabled` + `USB Mode = USB-OTG (TinyUSB)`, so the native USB
enumerates as a **composite CDC + HID** device — the terminal and the "it types by itself" demo
share one cable. After flashing, open the terminal on the **native USB** port. If the HID demo
isn't detected by your computer, unplug/replug the native-USB cable once after flashing.

## Open the terminal

Use a **real ANSI terminal** — the Arduino IDE Serial Monitor does **not** render ANSI colors.

```bash
screen /dev/cu.usbmodem* 115200      # quit with ctrl-a then k
# or
./build.sh monitor                    # uses arduino-cli monitor
# or minicom / iTerm / PuTTY at 115200 8N1
```

You'll get a full-screen menu. Navigate with **↑/↓ or number keys 1–7**, **Enter** to launch,
and **Esc** returns to the menu from any demo. Each demo shows its own keys in the footer bar.

The firmware redraws the current screen when a terminal (re)connects, so you'll see the menu
as soon as you attach — no keypress needed.

## First boot / "the screen is blank"

Because this build runs over the **native USB (TinyUSB)** port, a software reset through that
port often lands the chip in ROM **download mode** instead of running the sketch (you'd see
`waiting for download` and the device stays enumerated as *"USB JTAG/serial debug unit"*
rather than *"ESP32S3_DEV"*). This is normal S3 native-USB behavior.

- **After flashing, tap the physical `RST` button once** (not `BOOT`) to boot the firmware.
  When it's running, it re-enumerates as **`ESP32S3_DEV`** and the port changes to something
  like `/dev/cu.usbmodem<MAC>` — open the terminal on that.
- **To re-flash:** `./build.sh upload`. If upload can't connect, hold `BOOT`, tap `RST`,
  release `BOOT` (manual download mode), flash, then tap `RST` to run.

## The onboard RGB LED (GPIO48 vs GPIO38)
- DevKitC-1 **v1.0** and most clones: **GPIO48**
- DevKitC-1 **v1.1**: **GPIO38**

The firmware defaults to GPIO48. If the LED stays dark, press **`p`** in the *RGB Light Show*
or *System Info* screen to switch to GPIO38 (saved to flash). *System Info* also has **`t`**
for a red/green/blue blink test.

## Instagram tips
- Film the **terminal window** — the WIDS alert panel going red, the packet monitor bars
  dancing as you walk around, and the Wi-Fi scanner signal bars are the money shots.
- The HID demo ("I plugged in a dev board and it started typing") is the classic viral clip —
  point it at an empty text document.
- Put the single LED in frame on *Rainbow* or *Police* for a synced accent.

## Project layout
```
s3demo/
  s3demo.ino      main dispatcher + registry        rgb.{h,cpp}   WS2812 effects engine
  config.{h,cpp}  pins + NVS-persisted LED pin       term.{h,cpp}  ANSI terminal engine
  screen.h        cooperative screen interface       menu.cpp      main menu
  rgb_show.cpp  wifi_scan.cpp  wifi_monitor.cpp  ble_scan.cpp  wids.cpp  usb_hid.cpp  sysinfo.cpp
build.sh          deps / compile / upload / monitor helper
```
