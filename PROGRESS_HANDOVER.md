# Cypherbox V2 Modular Refactor — Handoff Notes

## What Was Done

Pulled starbeam_v2's modular architecture into cypherbox (minus NRF24/CC1101 radio modules).

### Created Files

```
cypherbox/
├── cypherbox.ino           # Main sketch — rewritten to use all modules
├── config.h               # Hardware pins + constants (cypherbox + starbeam merged)
├── types.h                # AppState/MenuItem enums + structs
├── src/
│   ├── display.cpp/h       # OLED UI (from starbeam_v2)
│   ├── input.cpp/h        # Non-blocking button handling
│   ├── terminal.cpp/h      # Serial CLI (wifi_scan, ble_scan, help, etc.)
│   ├── wifi_scanner.cpp/h # WiFi network scanner + channel heatmap
│   ├── ble_scanner.cpp/h  # BLE device discovery
│   ├── cypherbox_webserver.cpp/h  # Captive-portal web dashboard
│   ├── packet_monitor.cpp/h # Passive packet monitor + PCAP recorder
│   ├── rfid_tools.cpp/h    # RFID identify/read/dump/write lab tools
│   ├── system_tools.cpp/h  # AP/WiFi/SD/stop-all utilities
│   ├── bluetooth_tools.cpp/h # Bluetooth Serial + safe test mode
│   └── Buffer.cpp/h       # Double-buffered PCAP writer (original)
├── AGENTS.md              # Updated contributor guide
└── PROGRESS_HANDOVER.md   # This file
```

### Installed Libraries (cloned to ~/Documents/Arduino/libraries/)

- `BLE-Keyboard` — from T-vK/ESP32-BLE-Keyboard
- `MFRC522` — from miguelbalboa/rfid
- `Adafruit_NeoPixel` — from adafruit/Adafruit_NeoPixel

### Key Merge Decisions

- **Class name kept as `StarbeamWebServer`** (not renamed) — used throughout
- **File renamed to `cypherbox_webserver.cpp/h`** to avoid conflict with ESP32's built-in `WebServer.h`
- **WiFi scanner**: Uses `WiFi.scanNetworks()` + `WiFi.RSSI()` — simpler than ESP-IDF API
- **authMode**: Changed from `wifi_auth_mode_t` to `uint8_t` to avoid ESP-IDF header dependency
- **BLE**: `isConnectable()` used instead of `getConnectable()`
- **Buffer.cpp**: Removed SPIFFS references, uses SD card only

---

## Remaining Build Errors (Last Known)

Resolved in this pickup:

1. **`ble_scanner.cpp`** — `BLEScanner::deviceCount` and `BLEScanner::devices` are now public static members so `MyBLECallback` can populate scan results.

2. **`cypherbox_webserver.cpp`** — Removed redundant `uint8_t` cast around `net->authMode`.

3. **`BleKeyboard.h`** — Removed the unused include from `cypherbox.ino`; the installed BLE-Keyboard library does not compile cleanly with ESP32 core 3.3.8 and the current sketch only has a placeholder `bt_hid` menu item.

4. **Linker issues** — Removed the duplicate `useSD` definition from `src/Buffer.cpp` and removed the old raw-frame attack shim/module from this non-attack build.

5. **Partition size** — Default `esp32:esp32:esp32` app partition is too small. Use `PartitionScheme=huge_app`.

6. **Attack archive** — The old WiFi attack module is preserved under `archive/disabled_modules/wifi_attack.*.disabled` for project history. It is outside `src/` and has a `.disabled` suffix so Arduino does not compile it.

---

## How to Fix and Complete

### Step 1: Clean Build Cache

```bash
rm -rf ~/Library/Arduino15/cache/*
rm -rf ~/Library/Arduino15/sketchBook/libraries/cypherbox
```

### Step 2: Fix ble_scanner.h — Make Static Members Public

```cpp
// In src/ble_scanner.h, change:
private:
    static BLEDeviceInfo devices[MAX_BLE_DEVICES];
    static int deviceCount;
    // ...

// TO:
public:
    static BLEDeviceInfo devices[MAX_BLE_DEVICES];
    static int deviceCount;
    // ...
```

### Step 3: Fix cypherbox_webserver.cpp

Find line with:
```cpp
String sec = getSecurityString((uint8_t)net->authMode);
```
Change to:
```cpp
String sec = getSecurityString(net->authMode);
```

### Step 4: Verify No Ghost Files

```bash
# Must show ONLY these two files:
ls src/cypherbox_webserver.cpp
ls src/cypherbox_webserver.h

# These must NOT exist:
ls src/webserver.cpp   # should fail
ls src/webserver.h    # should fail
```

### Step 5: Compile

```bash
arduino-cli compile --fqbn esp32:esp32:esp32:PartitionScheme=huge_app
```

Verified outcome: clean compile. Size from last successful build after V2 completion: `1880734 bytes (59%)` of `3145728 bytes`; globals `68936 bytes (21%)`.

---

## Original cypherbox Features Preserved

- `runWardriver()` — GPS + WiFi logging to SD card
- `RfidTools` — MFRC522 identify/read/dump/write lab workflow
- `SystemTools` — SD card list/read/delete utilities
- `runPartyLight()` / `runLightOff()` — NeoPixel control

## New Features Added from starbeam_v2

- WiFi Network Scanner (`wifi_scan`)
- WiFi Channel Heatmap (`wifi_heatmap`)
- BLE Device Scanner (`ble_scan`)
- Web Dashboard + Captive Portal (`web_on`)
- Passive packet monitor + WiFi sniffer views with optional PCAP recording to SD
- RFID identify, block read, SD dump/list, and controlled restore to blank/writable test cards
- SD list/read/delete utilities
- AP create/stop, WiFi join, stop-all cleanup, Bluetooth Serial, and safe BT HID test mode
- Attack/deauth/devil-twin style modes are disabled in this V2 build
- Serial CLI with `help`, `menu`, `status`, `wifi_list`, `ble_list`, `stop`, `wifi_join`, `rfid_dump`, `rfid_write`, `sd_read`, `packet_record`, `channel`

## Verify in cypherbox.ino

Make sure these includes are present (no duplicates):

```cpp
#include "src/display.h"
#include "src/input.h"
#include "src/terminal.h"
#include "src/wifi_scanner.h"
#include "src/ble_scanner.h"
#include "src/packet_monitor.h"
#include "src/rfid_tools.h"
#include "src/system_tools.h"
#include "src/bluetooth_tools.h"
#include "src/Buffer.h"
#include "src/cypherbox_webserver.h"   // <-- not webserver.h
```

---

## Flash Command (when ready)

```bash
arduino-cli upload --fqbn esp32:esp32:esp32:PartitionScheme=huge_app --port /dev/ttyUSB0
# Hold BOOT button (Pin 2) while uploading

arduino-cli monitor --port /dev/ttyUSB0 --baudrate 115200
```
