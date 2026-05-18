// payload_menu.h - On-device payload browser for BT HID.
// Built on top of cypherbox's Display + Input + SD.

#pragma once

#include <Arduino.h>

namespace payload_menu {

// One-shot initial scan of PAYLOAD_DIR (called from BtHid::init).
void init();

// Re-scan SD (called after web UI writes a payload).
void reload();

// Modal blocking browser. Returns when the user long-presses UP (back).
// Cooperative with BLE/web — uses non-blocking polling internally.
void run();

// Fire a payload by "<folder>/<name>" identifier (called from web UI).
void runByName(const char* path);

}  // namespace payload_menu
