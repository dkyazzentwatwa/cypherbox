# Repository Guidelines

Cypherbox V2 is a modular ESP32 cybersecurity toolkit. This guide covers the new architecture and development workflow.

## Project Structure

```
cypherbox/
├── cypherbox.ino           # Main sketch — entry point, setup, loop
├── config.h                # Hardware pin assignments and constants
├── types.h                 # Enums (AppState, MenuItem) and structs
├── src/
│   ├── display.cpp/h       # OLED UI rendering (from starbeam_v2)
│   ├── input.cpp/h         # Non-blocking button handling
│   ├── terminal.cpp/h      # Serial CLI interface
│   ├── wifi_scanner.cpp/h  # WiFi network + channel heatmap scanner
│   ├── ble_scanner.cpp/h   # BLE device discovery
│   ├── webserver.cpp/h     # Captive-portal web dashboard
│   ├── packet_monitor.cpp/h # Passive packet monitor + PCAP recorder
│   ├── rfid_tools.cpp/h    # RFID identify/read/dump/write lab tools
│   ├── system_tools.cpp/h  # AP/WiFi/SD/stop-all utilities
│   ├── bluetooth_tools.cpp/h # Bluetooth Serial + safe test mode
│   └── Buffer.cpp/h        # Double-buffered PCAP writer (original)
├── hardware/               # Schematics, gerbers, PCB design files
├── img/                    # Device photos and banner
├── LICENSE
└── README.md
```

## Build & Flash

Arduino/ESP32 project. Use Arduino IDE or `arduino-cli`:

```bash
# Compile
arduino-cli compile --fqbn esp32:esp32:esp32:PartitionScheme=huge_app

# Flash — hold BOOT button (Pin 2) while uploading
arduino-cli upload --fqbn esp32:esp32:esp32:PartitionScheme=huge_app --port /dev/ttyUSB0

# Monitor at 115200 baud
arduino-cli monitor --port /dev/ttyUSB0 --baudrate 115200
```

**Board**: ESP32-WROOM-32D/E. Use `PartitionScheme=huge_app`; the modular V2 firmware exceeds the default app partition.

## Pin Assignments

| Component       | Pins                                      |
|-----------------|-------------------------------------------|
| SD Card (SPI)   | CS=5, MOSI=23, MISO=19, SCK=18           |
| RFID MFRC522    | RST=25, SS=27, MOSI=23, MISO=19, SCK=18  |
| OLED SSD1306    | I2C (default)                             |
| Neopixel WS2812 | DI=26                                     |
| GPS Module      | TX=17, RX=16                             |
| Buttons         | Up=34, Down=35, Select=15, Home=2         |

**Boot mode**: Pin 2 (Home) must be held LOW during upload.

## Coding Style

- **Indent**: 2 spaces (Arduino default)
- **Enums**: `enum AppState { STATE_X, ... }` for state machine; `enum MenuItem { MENU_X, ... }` for menu entries
- **Naming**: `camelCase` for variables, `SCREAMING_SNAKE_CASE` for constants
- **Modules**: One class per `.cpp/h` pair in `src/`. Include `config.h` and `types.h` first.
- **Serial output**: `Serial.print*()` at 115200 baud for debug
- **Terminal CLI**: All serial commands routed through `Terminal::processInput()`

## Adding a New Feature

1. Add a `MenuItem` entry in `types.h`
2. Add the menu label in `src/display.cpp` (match enum order)
3. Add a command mapping in `src/terminal.cpp` `commands[]`
4. Implement the handler function in `cypherbox.ino`
5. Wire it into `executeSelectedMenuItem()` with a `case` block

## Terminal Commands

```bash
help          # Show all commands
menu          # List all menu items with numbers
status        # System info (heap, WiFi/BLE state)
stop          # Stop current operation
wifi_list     # List scanned WiFi networks
ble_list      # List scanned BLE devices
```

Feature commands: `wifi_scan`, `ble_scan`, `wifi_heatmap`, `packet_mon`, `wifi_sniff`, `web_on`, `web_off`, `rfid`, `read_blocks`, `rfid_dump`, `rfid_list`, `rfid_write <dump>`, `sd_list`, `sd_read <file>`, `sd_delete <file> confirm`, `wardriver`, `bt_serial`, `bt_hid`, etc.

Attack/deauth/devil-twin style modes are intentionally disabled in this V2 build.

## Web Server

Start via menu or `web_on` command. Connects to `http://<AP_IP>` (captive portal auto-redirects).

- `/` — Dashboard (WiFi/BLE counts, free heap)
- `/wifi` — WiFi network table
- `/ble` — BLE device table
- `/security` — Attack controls (legal warning page)
- `/api/wifi`, `/api/ble` — JSON data endpoints

Default AP: SSID=`Cypherbox`, Password=`cypherbox2024`, IP=`192.168.4.1`

## Dependencies

Libraries are included via `#include` at the top of `cypherbox.ino`. Install via Arduino Library Manager or `arduino-cli lib install`. Key libraries:

- Adafruit GFX + SSD1306 + NeoPixel
- MFRC522, TinyGPSPlus, RTClib
- ESP32 core WiFi/BLE/BluetoothSerial
- espsoftwareserial (for GPS if needed)

## Hardware Documentation

Schematics and PCB files in `hardware/`. Gerber files are production-ready for PCB fabrication.
