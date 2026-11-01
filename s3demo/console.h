#pragma once

// Line-based command console for phone BLE-serial apps (nRF Toolbox, LightBlue,
// Bluefruit, etc.). Phones type whole commands + Enter instead of arrow keys, so
// over BLE the device is driven by typed words (`help`, `wifi`, `run`, ...) rather
// than the single-key/arrow UI used on a USB terminal. Screen *output* still goes
// out as the plain-text snapshots built by `io`.
namespace console {
  void onConnect();   // send the welcome banner when a phone connects
  void service();     // call every loop(): drain + execute BLE command lines
}
