# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

**Cypherbox** is a modular ESP32-based cybersecurity and networking toolkit with an OLED UI, SD card logging, RFID tools, WiFi/BLE scanning, GPS wardriving, and packet monitoring. It combines original Cypherbox features with starbeam_v2 modules, minus radio-based attack modes.

Hardware: ESP32-WROOM-32, SSD1306 OLED, SD card module, MFRC522 RFID, GPS, Neopixel LED, three input buttons.

## Build & Compile

**Compile** (requires `arduino-cli`):
```bash
arduino-cli compile --fqbn esp32:esp32:esp32:PartitionScheme=huge_app
```

The **huge_app** partition scheme is required for the modular V2 firmware (do not omit).

**Important Upload Note**: Button HOME (Pin 2) must be held down during firmware upload to enter bootmode.

## Architecture & Module Structure

- **Main entry**: `cypherbox.ino` — includes all modules, sets up global state, and implements main application flow
- **config.h**: All hardware pin definitions (OLED, buttons, RFID, SD, GPS, etc.) and buffer/timing constants; update here if hardware pins change
- **types.h**: Application state machine (AppState enum), menu items (MenuItem enum), and data structures (WiFiNetworkInfo, ButtonState, etc.)
- **src/**: Modular implementation files, each with .h and .cpp pairs:
  - **display**: OLED UI, menu rendering, info screens; static methods via Display class
  - **terminal**: Serial CLI parsing; maps string commands to MenuItem enum values
  - **input**: Button debouncing and event handling (non-blocking)
  - **wifi_scanner**: WiFi network discovery and channel heatmap
  - **ble_scanner**: Bluetooth LE device scanning
  - **rfid_tools**: MFRC522 RFID reading, writing, and block dumps to SD
  - **packet_monitor**: WiFi sniffer and optional PCAP recording
  - **bluetooth_tools**: Bluetooth serial bridge and HID testing
  - **cypherbox_webserver**: Web UI for AP mode
  - **system_tools**: GPS wardriver, SD card operations, RTC management
  - **Buffer**: PCAP packet buffering

## Control Interfaces

**OLED Menu**: Three buttons (Up, Down, Select) navigate and enter modes; Home button is reserved for bootmode.

**Serial CLI**: Sends commands via serial (see README for full command list). Terminal class parses input, maps to MenuItem, and triggers state transitions. Key patterns:
- Commands can accept arguments (e.g., `wifi_join ssid password`)
- Commands can broadcast to OLED via Terminal::echoToSerial()
- Check Terminal::stopRequested() in loops for graceful exit

## Key Design Patterns

**State Machine**: Main loop in `cypherbox.ino` checks `currentState` (AppState enum) and calls appropriate mode handlers. Menu selection updates `currentState` and `selectedMenuItem`; serial commands can also trigger state changes via Terminal class.

**Non-Blocking Delays**: Use `nonBlockingDelay()` to allow button polling and serial processing during waits. Do NOT use `delay()` unless the entire application can pause (e.g., startup sequences).

**SD Card Initialization**: Check `sdInitialized` flag; re-init via `SD.begin(SD_CS)` only when needed. GPS and SD share serial/SPI buses and must be initialized in correct order.

**Shared SPI Bus**: RFID and SD card both use HSPI (pins 18, 19, 23) with different chip-selects (SS 27 for RFID, CS 5 for SD). Avoid simultaneous access.

**Display Regions**: Display class manages OLED state; call Display::init() once, then use static methods (drawMenu, displayInfo, etc.) throughout. The U8G2 wrapper and Adafruit_SSD1306 objects are exposed via getOled() / getU8g2() if direct low-level drawing is needed.

## Constants & Configuration

- **Debounce**: DEBOUNCE_MS (50 ms), LONG_PRESS_MS (1000 ms)
- **WiFi Scan**: MAX_NETWORKS (20), WIFI_SCAN_INTERVAL (10 sec), NUM_WIFI_CHANNELS (14)
- **BLE Scan**: MAX_BLE_DEVICES (20), BLE_SCAN_DURATION (5 sec)
- **Web Server**: Port 80, AP SSID "Cypherbox" (configurable in config.h)
- **Buffers**: BUF_LENGTH (128 for commands), BUF_SIZE (24 KB for PCAP), SNAP_LEN (2324 for packet data)

All in `config.h` and `types.h`; avoid magic numbers in implementation code.

## Dependencies

- **Arduino Core**: ESP32 board support (wifi, BLE, GPIO, SPI, UART)
- **Display**: Adafruit_GFX, Adafruit_SSD1306, U8g2_for_Adafruit_GFX
- **Sensors**: TinyGPSPlus (GPS), RTClib (RTC)
- **Storage**: SD library
- **RF**: WiFi, BLE (built-in)
- **RFID**: Not explicitly listed (likely MFRC522 library via Arduino IDE)
- **Neopixel**: Adafruit_NeoPixel
- **Web**: None (raw HTTP parsing in cypherbox_webserver.cpp)

Install via Arduino IDE library manager or PlatformIO.

## Testing & Debugging

- **Serial Monitor**: Connect at appropriate baud rate (typically 115200); type commands to test CLI
- **OLED**: Verify menu navigation and info screens during setup
- **Serial Logging**: Terminal class and modules can echo diagnostics via Serial.println()
- **Partition Scheme**: Errors about code size usually mean huge_app wasn't used during compile

## Notes for Development

- **Attack Modes Disabled**: Deauth, beacon flood, probe flood, and devil-twin are present in enum but intentionally not fully wired in v2 (disabled in the README)
- **Memory Constraints**: ESP32 has limited RAM; packet capture buffer (24 KB) and WiFi scanning (20 networks) are tuned for stability; increase carefully
- **RF Coexistence**: WiFi and BLE share hardware; running both simultaneously may reduce range/throughput
- **SD/GPS Conflict**: Serial1 (pins 16/17) is used for GPS; ensure no other code claims those pins
- **Partition Scheme**: The huge_app partition allocates ~3 MB for app code; if code grows beyond ~2.5 MB, compilation will fail (consider disabling unused features or splitting into two binaries)

## File Organization Note

- `archive/disabled_modules`: Contains older features not integrated in v2; reference only for context
- `hardware/`: Schematic and PCB files (KiCad, Gerber); electrical reference for pin conflicts/power distribution

## Commit & Testing Workflow

- Compile with `arduino-cli` before committing; include partition scheme flag
- Test on device: boot, menu navigation, at least one feature per module touched
- Serial commands and state transitions should not hang the UI (use non-blocking delays and poll-based architectures)
