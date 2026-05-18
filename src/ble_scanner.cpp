// ble_scanner.cpp - BLE Scanner Module Implementation for Cypherbox V2.
// Ported from stock Bluedroid <BLEDevice.h> to NimBLE-Arduino so the
// firmware can also link the BT-HID keyboard (which uses NimBLE).

#include "ble_scanner.h"
#include "display.h"
#include "input.h"
#include "config.h"
#include "terminal.h"

// Static members
BLEDeviceInfo BLEScanner::devices[MAX_BLE_DEVICES];
int BLEScanner::deviceCount = 0;
int BLEScanner::currentDeviceIndex = 0;
bool BLEScanner::detailMode = false;
int BLEScanner::detailScrollOffset = 0;
unsigned long BLEScanner::lastScanTime = 0;
NimBLEScan* BLEScanner::pBLEScan = nullptr;

namespace {

// NimBLE callback API: onResult takes a const NimBLEAdvertisedDevice*
// (whose lifetime is tied to the scan results), so we copy out the
// relevant fields immediately.
class CypherScanCallbacks : public NimBLEScanCallbacks {
    void onResult(const NimBLEAdvertisedDevice* dev) override {
        if (!dev) return;
        if (BLEScanner::deviceCount >= MAX_BLE_DEVICES) return;
        BLEDeviceInfo info = {};
        info.payload = nullptr;

        std::string name = dev->getName();
        strncpy(info.name, name.c_str(), sizeof(info.name) - 1);
        std::string addr = dev->getAddress().toString();
        strncpy(info.address, addr.c_str(), sizeof(info.address) - 1);
        info.rssi = dev->getRSSI();
        info.connectable = dev->isConnectable();

        BLEScanner::devices[BLEScanner::deviceCount++] = info;
    }
};

CypherScanCallbacks bleCallback;

}  // namespace

void BLEScanner::init() {
    NimBLEDevice::init("Cypherbox");
    pBLEScan = NimBLEDevice::getScan();
    pBLEScan->setActiveScan(true);
    pBLEScan->setInterval(100);
    pBLEScan->setWindow(99);
    pBLEScan->setScanCallbacks(&bleCallback, false);
    deviceCount = 0;
    currentDeviceIndex = 0;
    detailMode = false;
    detailScrollOffset = 0;
    lastScanTime = 0;
    Serial.println("BLE Scanner initialized (NimBLE)");
}

void BLEScanner::deinit() {
    for (int i = 0; i < deviceCount; i++) {
        if (devices[i].payload) {
            delete[] devices[i].payload;
            devices[i].payload = nullptr;
        }
    }
    // NB: do NOT call NimBLEDevice::deinit() here — the BT-HID keyboard
    // shares the stack and must keep advertising after the scanner exits.
    if (pBLEScan) pBLEScan->stop();
    pBLEScan = nullptr;
    deviceCount = 0;
}

const BLEDeviceInfo* BLEScanner::getDevice(int index) {
    if (index >= 0 && index < deviceCount) return &devices[index];
    return nullptr;
}

void BLEScanner::performScan() {
    if (!pBLEScan) return;
    deviceCount = 0;
    // NimBLE 2.x: getResults takes scan duration in MILLISECONDS.
    NimBLEScanResults results = pBLEScan->getResults(BLE_SCAN_DURATION * 1000, false);
    int n = (int)results.getCount();
    if (n > MAX_BLE_DEVICES) n = MAX_BLE_DEVICES;
    deviceCount = n;
    for (int i = 0; i < n; i++) {
        const NimBLEAdvertisedDevice* dev = results.getDevice(i);
        if (!dev) continue;
        devices[i] = {};
        devices[i].payload = nullptr;
        std::string name = dev->getName();
        strncpy(devices[i].name, name.c_str(), sizeof(devices[i].name) - 1);
        std::string addr = dev->getAddress().toString();
        strncpy(devices[i].address, addr.c_str(), sizeof(devices[i].address) - 1);
        devices[i].rssi = dev->getRSSI();
        devices[i].connectable = dev->isConnectable();
    }
    pBLEScan->clearResults();
    lastScanTime = millis();
    Serial.printf("BLE Scan: Found %d devices\n", deviceCount);
}

void BLEScanner::runScanner() {
    if (!pBLEScan) init();
    performScan();
    displayListMode();
    delay(250);

    while (true) {
        if (millis() - lastScanTime >= BLE_SCAN_INTERVAL) {
            performScan();
            if (detailMode) displayDetailMode();
            else displayListMode();
        }
        if (Input::isButtonPressed(BUTTON_UP)) {
            if (detailMode) {
                detailScrollOffset = max(0, detailScrollOffset - 1);
            } else {
                currentDeviceIndex = (currentDeviceIndex - 1 + max(deviceCount, 1)) % max(deviceCount, 1);
            }
            if (detailMode) displayDetailMode();
            else displayListMode();
            delay(200);
        }
        if (Input::isButtonPressed(BUTTON_DOWN)) {
            if (detailMode) {
                detailScrollOffset++;
            } else {
                currentDeviceIndex = (currentDeviceIndex + 1) % max(deviceCount, 1);
            }
            if (detailMode) displayDetailMode();
            else displayListMode();
            delay(200);
        }
        if (Input::isButtonPressed(BUTTON_SELECT)) {
            if (detailMode) {
                delay(200);
                return;
            }
            detailMode = true;
            detailScrollOffset = 0;
            displayDetailMode();
            delay(200);
        }
    }
}

void BLEScanner::displayListMode() {
    char line1[32], line2[32], line3[32];
    snprintf(line1, sizeof(line1), "BLE: %d/%d", currentDeviceIndex + 1, deviceCount);
    if (deviceCount > 0 && currentDeviceIndex < deviceCount) {
        BLEDeviceInfo& dev = devices[currentDeviceIndex];
        snprintf(line2, sizeof(line2), "%s", dev.name[0] ? dev.name : "(no name)");
        snprintf(line3, sizeof(line3), "%s rssi:%d", dev.address, dev.rssi);
    } else {
        snprintf(line2, sizeof(line2), "No devices");
        snprintf(line3, sizeof(line3), "");
    }
    Display::displayInfo(line1, line2, line3, "[UP/DN]=nav [SEL]=detail");
}

void BLEScanner::displayDetailMode() {
    if (deviceCount == 0 || currentDeviceIndex >= deviceCount) {
        Display::displayInfo("BLE Detail", "No device", "", "[SEL]=list");
        return;
    }
    BLEDeviceInfo& dev = devices[currentDeviceIndex];
    Display::displayInfo(String("BLE: ") + dev.address,
                          String("Name: ") + (dev.name[0] ? dev.name : "(none)"),
                          String("RSSI: ") + dev.rssi + " dBm",
                          "[DN]=scroll [SEL]=list");
}

void BLEScanner::listAll() {
    for (int i = 0; i < deviceCount; i++) {
        BLEDeviceInfo& dev = devices[i];
        Serial.printf("[%d] %s %s rssi:%d\n",
            i, dev.name[0] ? dev.name : "(no name)", dev.address, dev.rssi);
    }
}
