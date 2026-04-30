// types.h - Type Definitions for Cypherbox V2

#ifndef TYPES_H
#define TYPES_H

#include <Arduino.h>

// ============================================================================
// Application State Machine
// ============================================================================

enum AppState {
  STATE_MENU,
  // Cypherbox Original
  STATE_PACKET_MON,
  STATE_WIFI_SNIFFER,
  STATE_AP_SCAN,
  STATE_AP_JOIN,
  STATE_AP_CREATE,
  STATE_STOP_AP,
  STATE_STOP_SERVER,
  STATE_BT_SCAN,
  STATE_BT_SERIAL,
  STATE_BT_HID,
  STATE_DEVIL_TWIN,
  STATE_RFID,
  STATE_FILES,
  STATE_READ_FILES,
  STATE_READ_BLOCKS,
  STATE_WARDRIVER,
  // Starbeam V2 Modules
  STATE_WIFI_SCAN,
  STATE_WIFI_HEATMAP,
  STATE_BLE_SCAN,
  STATE_WEBSERVER,
  // Security Testing States
  STATE_SEC_DEAUTH_TARGET,
  STATE_SEC_DEAUTH_ALL,
  STATE_SEC_BEACON_FLOOD,
  STATE_SEC_PROBE_FLOOD,
  STATE_SEC_PMKID_CAPTURE,
  // Recording
  STATE_REC_RAW,
  STATE_PLAY_RAW,
  STATE_SHOW_RAW,
  STATE_SHOW_BUFF,
  STATE_FLUSH_BUFF,
  // Utility
  STATE_STOP_ALL,
  STATE_GET_RSSI,
  STATE_SETTINGS,
  STATE_HELP,
  // Captive Portal
  STATE_CAPTIVE_PORTAL
};

// ============================================================================
// Menu Items (unified)
// ============================================================================

enum MenuItem {
  // Cypherbox Original
  PACKET_MON,
  WIFI_SNIFF,
  AP_SCAN,
  AP_JOIN,
  AP_CREATE,
  STOP_AP,
  STOP_SERVER,
  BT_SCAN,
  BT_CREATE,
  BT_SERIAL_CMD,
  BT_HID,
  DEVIL_TWIN,
  RFID,
  READ_BLOCKS,
  WARDRIVER,
  PARTY_LIGHT,
  LIGHTOFF,
  FILES,
  READ_FILES,
  // Starbeam V2 Modules
  WIFI_SCAN,
  WIFI_HEATMAP,
  BLE_SCAN,
  WEBSERVER_ON,
  WEBSERVER_OFF,
  WEBSERVER_STATUS,
  // Security Testing
  SEC_DEAUTH_TARGET,
  SEC_DEAUTH_ALL,
  SEC_BEACON_FLOOD,
  SEC_PROBE_FLOOD,
  SEC_PMKID_CAPTURE,
  // Recording & Utility
  REC_RAW,
  PLAY_RAW,
  SHOW_RAW,
  SHOW_BUFF,
  GET_RSSI,
  FLUSH_BUFF,
  STOP_ALL,
  SETTINGS,
  HELP,
  // Captive Portal
  CAPTIVE_PORTAL,
  CAPTIVE_PORTAL_OFF,
  CAPTIVE_PORTAL_STATUS,
  NUM_MENU_ITEMS
};

// ============================================================================
// Signal Information Structure
// ============================================================================

struct SignalInfo {
  float frequency;
  float rssi;
  unsigned long timestamp;
};

// ============================================================================
// V2-Specific Types
// ============================================================================

enum OperationStatus {
  OP_IDLE,
  OP_RUNNING,
  OP_PAUSED,
  OP_COMPLETE,
  OP_ERROR
};

// Display update region for dirty tracking
struct DisplayRegion {
  uint16_t x, y, width, height;
  bool dirty;
};

// Button state for non-blocking debouncing
struct ButtonState {
  uint8_t pin;
  bool currentState;
  bool lastState;
  uint32_t lastChangeTime;
  bool longPressHandled;
};

// Button events for event queue
enum ButtonEvent {
  BTN_NONE,
  BTN_UP_PRESS,
  BTN_UP_LONG,
  BTN_DOWN_PRESS,
  BTN_DOWN_LONG,
  BTN_SELECT_PRESS,
  BTN_SELECT_LONG
};

// WiFi Network Info (starbeam_v2)
struct WiFiNetworkInfo {
  String ssid;
  String bssid;
  int32_t rssi;
  uint8_t channel;
  uint8_t authMode;
};

#endif // TYPES_H
