#include "bleserial.h"
#include "io.h"
#include <NimBLEDevice.h>

// Nordic UART Service — the de-facto standard every phone BLE-serial app speaks.
#define NUS_SERVICE "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define NUS_RX      "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"  // phone -> device (write)
#define NUS_TX      "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"  // device -> phone (notify)

namespace {
  NimBLEServer*         server = nullptr;
  NimBLECharacteristic* txChar = nullptr;
  volatile bool         isConnected = false;
  volatile uint16_t     negMTU = 23;           // negotiated ATT MTU (default 23)

  class ServerCB : public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* s, NimBLEConnInfo& info) override {
      isConnected = true;
      negMTU = info.getMTU();
      io.onBleConnect();                       // push a fresh frame to the phone
    }
    void onDisconnect(NimBLEServer* s, NimBLEConnInfo& info, int reason) override {
      isConnected = false;
      NimBLEDevice::startAdvertising();        // become findable again
    }
    void onMTUChange(uint16_t mtu, NimBLEConnInfo& info) override { negMTU = mtu; }
  };

  class RxCB : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* c, NimBLEConnInfo& info) override {
      NimBLEAttValue v = c->getValue();
      if (v.length()) ioBleRxPush(v.data(), v.length());
    }
  };

  ServerCB srvCb;
  RxCB     rxCb;
}

namespace bleserial {

void begin() {
  NimBLEDevice::init("littleHAKR");
  NimBLEDevice::setMTU(247);                   // request a larger MTU (phone decides)

  server = NimBLEDevice::createServer();
  server->setCallbacks(&srvCb, false);

  NimBLEService* svc = server->createService(NUS_SERVICE);
  txChar = svc->createCharacteristic(NUS_TX, NIMBLE_PROPERTY::NOTIFY);
  NimBLECharacteristic* rx =
      svc->createCharacteristic(NUS_RX, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR);
  rx->setCallbacks(&rxCb);
  svc->start();

  NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
  adv->addServiceUUID(NUS_SERVICE);
  adv->setName("littleHAKR");
  adv->enableScanResponse(true);
  NimBLEDevice::startAdvertising();
}

bool connected() { return isConnected; }

// Chunked NOTIFY: split the snapshot into (MTU-3)-byte writes with a brief gap so
// the controller can drain. Bounded so a congested link can't stall the UI.
void send(const uint8_t* buf, size_t len) {
  if (!isConnected || !txChar) return;
  int chunk = (int)negMTU - 3;
  if (chunk < 20)  chunk = 20;
  if (chunk > 180) chunk = 180;
  size_t off = 0;
  while (off < len && isConnected) {
    int n = (int)(len - off);
    if (n > chunk) n = chunk;
    if (!txChar->notify(buf + off, n)) { delay(15); if (!txChar->notify(buf + off, n)) break; }
    off += n;
    if (off < len) delay(12);                  // let the stack breathe between chunks
  }
}

} // namespace bleserial
