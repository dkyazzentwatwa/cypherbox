// wardriver.h - Passive WiFi + BLE WiGLE logger for Cypherbox V2

#ifndef WARDRIVER_H
#define WARDRIVER_H

#include <Arduino.h>
#include <TinyGPSPlus.h>
#include <HardwareSerial.h>
#include <BLEDevice.h>
#include <BLEScan.h>
#include <WiFi.h>
#include <SD.h>
#include "../config.h"

struct WardriverFix {
    bool valid = false;
    bool fresh = false;
    double latitude = 0.0;
    double longitude = 0.0;
    double altitudeMeters = 0.0;
    double hdop = 99.9;
    uint32_t ageMs = UINT32_MAX;
    uint32_t charsProcessed = 0;
    uint8_t satellites = 0;
    char timestamp[24] = "1970-01-01 00:00:00";
    char filenameStamp[24] = "NOFIX";
};

struct WardriverTopSignal {
    char id[18] = "";
    char label[33] = "";
    int32_t rssi = -128;
    int32_t channel = 0;
};

struct WardriverStats {
    uint32_t scans = 0;
    uint32_t gpsWaits = 0;
    uint32_t wifiRows = 0;
    uint32_t bleRows = 0;
    uint32_t wifiSeen = 0;
    uint32_t bleSeen = 0;
    uint32_t uniqueWifi = 0;
    uint32_t uniqueBle = 0;
    uint32_t openWifi = 0;
    uint32_t securedWifi = 0;
    uint32_t hiddenWifi = 0;
    uint32_t sdErrors = 0;
    uint32_t rotations = 0;
    char activeLog[40] = "";
    char activeSummary[40] = "";
    char lastError[40] = "";
    WardriverTopSignal topWifi[5];
    WardriverTopSignal topBle[5];
};

class Wardriver {
public:
    static void run();

private:
    static const uint8_t MIN_FIX_SATELLITES = 4;
    static const uint32_t GPS_FIX_STALE_MS = 10000;
    static const uint32_t NORMAL_SCAN_INTERVAL_MS = 15000;
    static const uint32_t FAST_SCAN_INTERVAL_MS = 5000;
    static const uint32_t DISPLAY_PAGE_MS = 7000;
    static const uint32_t SUMMARY_INTERVAL_MS = 30000;
    static const uint32_t LOG_ROTATE_BYTES = 4UL * 1024UL * 1024UL;
    static const uint8_t LOG_FLUSH_ROWS = 20;
    static const uint16_t MAX_UNIQUE_WIFI = 450;
    static const uint16_t MAX_UNIQUE_BLE = 200;
    static const uint8_t TOP_SIGNAL_COUNT = 5;

    static TinyGPSPlus gps;
    static HardwareSerial gpsSerial;
    static BLEScan* bleScan;
    static WardriverStats stats;
    static char knownWifi[MAX_UNIQUE_WIFI][18];
    static char knownBle[MAX_UNIQUE_BLE][18];
    static uint16_t knownWifiCount;
    static uint16_t knownBleCount;
    static uint8_t page;
    static bool loggingEnabled;
    static bool bleEnabled;
    static bool fastScan;
    static bool immediateScan;
    static bool stopRequested;
    static uint8_t rowsSinceFlush;
    static uint32_t lastScanMs;
    static uint32_t lastDisplayMs;
    static uint32_t lastPageMs;
    static uint32_t lastSummaryMs;
    static uint32_t lastButtonMs;
    static char serialBuffer[96];
    static uint8_t serialIndex;

    static bool begin();
    static void end();
    static void resetSession();
    static void updateGps();
    static WardriverFix currentFix();
    static bool isGpsDateTimeValid();
    static void formatGpsTimestamp(char* out, size_t len);
    static void formatFilenameStamp(char* out, size_t len);
    static void handleButtons();
    static void processSerial();
    static void handleSerialLine(String line);
    static void printStatus(const WardriverFix& fix);
    static void printGps(const WardriverFix& fix);
    static bool freshFixOrWait(const WardriverFix& fix);
    static bool ensureLog(const WardriverFix& fix);
    static bool createLog(const WardriverFix& fix);
    static bool rotateLog(const WardriverFix& fix);
    static uint32_t activeLogBytes();
    static void writeHeader(File& file);
    static void scanWifi(const WardriverFix& fix);
    static void scanBle(const WardriverFix& fix);
    static bool writeWifiRow(File& file, int index, const WardriverFix& fix);
    static bool writeBleRow(File& file, BLEAdvertisedDevice& device, const WardriverFix& fix);
    static void maybeWriteSummary(const WardriverFix& fix);
    static bool writeSummary(const WardriverFix& fix);
    static void render(const WardriverFix& fix);
    static void renderGps(const WardriverFix& fix);
    static void renderWifi();
    static void renderBle();
    static void renderTop();
    static void renderStorage();
    static void setError(const char* message);
    static bool noteUnique(char list[][18], uint16_t& count, uint16_t maxCount, const char* id);
    static void updateTop(WardriverTopSignal top[], const char* id, const char* label, int32_t rssi, int32_t channel);
    static String csvEscape(const String& value);
    static const char* authMode(wifi_auth_mode_t auth);
    static int32_t channelFrequency(int32_t channel);
    static void printClipped(const char* text, uint8_t maxChars);
};

#endif
