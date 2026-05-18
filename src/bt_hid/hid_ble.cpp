// hid_ble.cpp - BLE-HID keyboard wrapper for Cypherbox V2
// Vendored from ESP32_BT_HID/hid_ble.cpp

#include "hid_ble.h"
#include "bt_hid_config.h"
#include <HijelHID_BLEKeyboard.h>

namespace {
HijelHID_BLEKeyboard kb(BT_HID_DEVICE_NAME, BT_HID_DEVICE_MANUF, BT_HID_DEVICE_BATTERY);
}

namespace hidx {

void init(const char* deviceName, const char* manufacturer, uint8_t battery) {
    (void)deviceName; (void)manufacturer; (void)battery;
    kb.begin();
}

void tick() {}

bool isConnected() { return kb.isConnected(); }

void writeChar(char c) {
    if (!kb.isConnected()) return;
    kb.write((uint8_t)c);
}

void writeString(const char* s) {
    if (!kb.isConnected() || !s) return;
    while (*s) {
        kb.write((uint8_t)*s);
        delay(BT_HID_KEY_INTERCHAR_MS);
        ++s;
    }
}

void tap(uint8_t keycode, uint8_t modifiers) {
    if (!kb.isConnected()) return;
    kb.tap(keycode, modifiers);
}

}  // namespace hidx
