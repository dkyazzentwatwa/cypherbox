// bt_hid.h - BLE-HID payload deck entry point for Cypherbox V2.
// Replaces the old BluetoothTools::runHidSafeTest stub.

#ifndef BT_HID_H
#define BT_HID_H

#include <Arduino.h>

class BtHid {
public:
    // One-shot init: brings up BLE-HID advertising and the Wi-Fi payload
    // web editor. Must be called once from setup() after SD is ready.
    static void init();

    // Modal payload browser: drives an Up/Down/Select menu on the OLED
    // (SD payloads grouped by OS folder), previews on Select, fires on
    // second Select. Long-press UP exits back to the main menu.
    static void runPayloadMenu();

    // Fire a payload by "<folder>/<name>" identifier (used by the web UI).
    static void runByName(const char* path);

    // Refresh the payload list cache from SD (called after a web POST).
    static void reload();

    static bool isConnected();
    static bool isInited() { return inited; }

private:
    static bool inited;
};

#endif
