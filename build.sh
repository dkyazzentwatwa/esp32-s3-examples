#!/usr/bin/env bash
# s3demo build helper — ESP32-S3-DevKitC-1 / generic (arduino-cli)
set -euo pipefail

HERE="$(cd "$(dirname "$0")" && pwd)"
SKETCH="$HERE/s3demo"

# USBMode=default   -> USB-OTG (TinyUSB), required for the HID keyboard demo
# CDCOnBoot=cdc     -> Serial runs over native USB (composite with HID = one cable)
# huge_app          -> WiFi + BLE + TinyUSB don't fit the default 1.2MB app slot
FQBN="esp32:esp32:esp32s3:USBMode=default,CDCOnBoot=cdc,FlashSize=4M,PartitionScheme=huge_app,PSRAM=disabled"

ensure_deps() {
  arduino-cli core update-index
  arduino-cli core install esp32:esp32
  arduino-cli lib install "Adafruit NeoPixel"
  arduino-cli lib install "NimBLE-Arduino"
}

find_port() { arduino-cli board list | awk '/usbmodem|usbserial|wchusbserial|SLAB/{print $1; exit}'; }

cmd="${1:-compile}"
case "$cmd" in
  deps)    ensure_deps ;;
  compile) arduino-cli compile --fqbn "$FQBN" "$SKETCH" ;;
  upload)
    PORT="${2:-$(find_port)}"
    [ -z "$PORT" ] && { echo "No board port found. Plug in via the UART port, then: $0 upload /dev/cu.xxx"; exit 1; }
    arduino-cli compile --fqbn "$FQBN" "$SKETCH"
    arduino-cli upload --fqbn "$FQBN" -p "$PORT" "$SKETCH"
    echo
    echo "Flashed. Now open the ANSI terminal on the NATIVE USB port (not the UART port):"
    echo "  screen /dev/cu.usbmodem* 115200      # ctrl-a k to quit"
    ;;
  monitor)
    PORT="${2:-$(find_port)}"
    [ -z "$PORT" ] && { echo "No port found. Pass one: $0 monitor /dev/cu.xxx"; exit 1; }
    arduino-cli monitor -p "$PORT" -c baudrate=115200 ;;
  *) echo "usage: $0 {deps|compile|upload [port]|monitor [port]}"; exit 1 ;;
esac
