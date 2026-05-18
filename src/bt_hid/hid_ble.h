// hid_ble.h - BLE-HID keyboard wrapper for Cypherbox V2
// Vendored from ESP32_BT_HID/hid.h

#pragma once

#include <Arduino.h>

namespace hidx {

void init(const char* deviceName, const char* manufacturer, uint8_t battery);
void tick();
bool isConnected();

void writeChar(char c);
void writeString(const char* s);
void tap(uint8_t keycode, uint8_t modifiers = 0);

}  // namespace hidx
