// ble_scanner.h - BLE Scanner Module for Cypherbox V2

#ifndef BLE_SCANNER_H
#define BLE_SCANNER_H

#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include "../config.h"

// BLE Device Info structure
struct BLEDeviceInfo {
    char name[32];
    char address[18];
    int rssi;
    uint8_t* payload;
    size_t payloadLength;
    uint8_t addressType;
    bool connectable;
};

class BLEScanner {
public:
    static void init();
    static void deinit();
    static void runScanner();
    static void listAll();
    static int getDeviceCount() { return deviceCount; }
    static const BLEDeviceInfo* getDevice(int index);
    static bool isInitialized() { return pBLEScan != nullptr; }
    static void performScan();

    static BLEDeviceInfo devices[MAX_BLE_DEVICES];
    static int deviceCount;

private:
    static void displayListMode();
    static void displayDetailMode();

    static int currentDeviceIndex;
    static bool detailMode;
    static int detailScrollOffset;
    static unsigned long lastScanTime;
    static BLEScan* pBLEScan;
};

#endif
