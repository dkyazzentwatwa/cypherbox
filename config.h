// config.h - Hardware Configuration for Cypherbox V2
// Unified config combining cypherbox original + starbeam_v2 modules

#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ============================================================================
// Display & UI Pin Definitions (Cypherbox Original)
// ============================================================================

#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define OLED_I2C_ADDR 0x3C

// LED / Neopixel
#define LED_PIN 26

// Buttons (Cypherbox original mapping)
#define BUTTON_UP     34
#define BUTTON_DOWN   35
#define BUTTON_SELECT 15
#define BUTTON_HOME   2   // Boot mode pin (hold for upload)

// SD Card Module (SPI - shared with RFID)
#define SD_CS         5
#define SD_MOSI       23
#define SD_MISO       19
#define SD_SCK        18

// RFID MFRC522 Module (SPI - shared with SD)
#define RFID_RST      25
#define RFID_SS       27
#define RFID_MOSI     23
#define RFID_MISO     19
#define RFID_SCK      18

// GPS Module
#define GPS_TX        17
#define GPS_RX        16

// ============================================================================
// Display & UI Pin Definitions (Starbeam V2 additions)
// ============================================================================

// Overwrite with starbeam pinout for unified modules
// Cypherbox pins take priority where both exist
#define BUTTON_UP_STARBEAM     39
#define BUTTON_DOWN_STARBEAM   34
#define BUTTON_SELECT_STARBEAM 36

// ============================================================================
// SPI Bus Definitions
// ============================================================================

// HSPI (shared SPI bus for RFID + SD)
#define HSPI_SCK   18
#define HSPI_MISO  19
#define HSPI_MOSI  23
#define HSPI_SS    5

// ============================================================================
// Buffer & Memory Sizes
// ============================================================================

#define CCBUFFERSIZE         64      // CC1101 buffer size
#define RECORDINGBUFFERSIZE  4096    // Recording buffer size
#define BUF_LENGTH           128     // Command buffer length
#define MAX_SIGNALS          4       // Maximum signals to track

// PCAP Buffer (from Buffer.h)
#define BUF_SIZE        24 * 1024
#define SNAP_LEN        2324        // max len of each received packet

// ============================================================================
// Button & Timing Constants
// ============================================================================

#define DEBOUNCE_MS        50       // Debounce time (non-blocking)
#define LONG_PRESS_MS      1000     // Long press detection threshold
#define DISPLAY_REFRESH_MS 50       // Display refresh interval
#define BUTTON_POLL_MS     10       // Button polling interval

// ============================================================================
// Web Server Configuration (from starbeam_v2)
// ============================================================================

#define WEB_SERVER_PORT      80
#define AP_SSID              "Cypherbox"
#define AP_PASSWORD          "cypherbox2024"
#define AP_CHANNEL           1
#define AP_HIDDEN            false
#define AP_MAX_CONNECTIONS   4

// ============================================================================
// WiFi Scanner Configuration
// ============================================================================

#define MAX_NETWORKS         20
#define WIFI_SCAN_INTERVAL   10000  // 10 seconds
#define HEATMAP_SCAN_INTERVAL 2000  // 2 seconds
#define NUM_WIFI_CHANNELS     14

// ============================================================================
// BLE Scanner Configuration
// ============================================================================

#define MAX_BLE_DEVICES       20
#define BLE_SCAN_DURATION     5
#define BLE_SCAN_INTERVAL     10000 // 10 seconds

// ============================================================================
// WiFi Attack Configuration (from starbeam_v2)
// ============================================================================

#define NUM_FRAMES_PER_ATTACK   16
#define MAX_TARGET_NETWORKS     20
#define MAX_FAKE_APS            50
#define PMKID_TIMEOUT           120000 // 2 minutes
#define MAX_PMKID_HISTORY        10
#define MAX_BEACON_SSIDS         20
#define BEACON_FRAME_SIZE        128

#endif // CONFIG_H
