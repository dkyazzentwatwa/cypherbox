// ble_scanner.cpp - BLE Scanner Module Implementation for Cypherbox V2

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
BLEScan* BLEScanner::pBLEScan = nullptr;

class MyBLECallback : public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
        if (BLEScanner::deviceCount >= MAX_BLE_DEVICES) return;
        BLEDeviceInfo info = {};
        info.payload = nullptr;
        
        String name = advertisedDevice.getName().c_str();
        strncpy(info.name, name.c_str(), sizeof(info.name) - 1);
        String addr = advertisedDevice.getAddress().toString().c_str();
        strncpy(info.address, addr.c_str(), sizeof(info.address) - 1);
        info.rssi = advertisedDevice.getRSSI();
        info.connectable = advertisedDevice.isConnectable();
        
        BLEScanner::devices[BLEScanner::deviceCount++] = info;
    }
};

static MyBLECallback bleCallback;

void BLEScanner::init() {
    BLEDevice::init("Cypherbox");
    pBLEScan = BLEDevice::getScan();
    pBLEScan->setActiveScan(true);
    pBLEScan->setInterval(100);
    pBLEScan->setWindow(99);
    pBLEScan->setAdvertisedDeviceCallbacks(&bleCallback);
    deviceCount = 0;
    currentDeviceIndex = 0;
    detailMode = false;
    detailScrollOffset = 0;
    lastScanTime = 0;
    Serial.println("BLE Scanner initialized");
}

void BLEScanner::deinit() {
    for (int i = 0; i < deviceCount; i++) {
        if (devices[i].payload) {
            delete[] devices[i].payload;
            devices[i].payload = nullptr;
        }
    }
    BLEDevice::deinit();
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
    BLEScanResults* results = pBLEScan->start(BLE_SCAN_DURATION);
    deviceCount = min(results->getCount(), MAX_BLE_DEVICES);
    for (int i = 0; i < deviceCount; i++) {
        BLEAdvertisedDevice dev = results->getDevice(i);
        devices[i].payload = nullptr;
        String name = dev.getName().c_str();
        strncpy(devices[i].name, name.c_str(), sizeof(devices[i].name) - 1);
        String addr = dev.getAddress().toString().c_str();
        strncpy(devices[i].address, addr.c_str(), sizeof(devices[i].address) - 1);
        devices[i].rssi = dev.getRSSI();
        devices[i].connectable = dev.isConnectable();
    }
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
            if (detailMode) displayDetailMode();
            else displayListMode();
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
