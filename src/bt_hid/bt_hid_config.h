// bt_hid_config.h - Configuration constants for the BT HID subsystem.
// Kept separate from the global cypherbox config.h so the vendored HID
// module stays self-contained.

#pragma once

#define BT_HID_DEVICE_NAME      "cypherbox-hid"
#define BT_HID_DEVICE_MANUF     "cypher"
#define BT_HID_DEVICE_BATTERY   100

// Inter-keystroke delay when typing strings (ms).
#define BT_HID_KEY_INTERCHAR_MS 25

// Per-action breathing room between non-DELAY DuckyScript lines.
#define BT_HID_MIN_ACTION_GAP_MS 8

// SD root for payloads (matches ESP32_BT_HID's cypherbox profile).
#define BT_HID_PAYLOAD_DIR      "/cypherbox/payloads"

// Default payload seeded on first boot if PAYLOAD_DIR is empty.
#define BT_HID_DEFAULT_PAYLOAD_NAME "rickroll.duck"
#define BT_HID_DEFAULT_PAYLOAD_FOLDER "macos"

// Wi-Fi AP for the payload web editor.
#define BT_HID_AP_SSID_PREFIX   "cypherbox-"
#define BT_HID_AP_PASSWORD      "duckduck"
#define BT_HID_HTTP_PORT        80
