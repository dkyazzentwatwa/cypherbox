// wifi_scanner.h - WiFi Scanner Module for Cypherbox V2

#ifndef WIFI_SCANNER_H
#define WIFI_SCANNER_H

#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include "display.h"
#include "../config.h"

class WiFiScanner {
public:
    static void init();
    static void deinit();
    static void runScanner();
    static void runHeatmap();
    static void listAll();
    static int getNetworkCount() { return networkCount; }
    static const WiFiNetworkInfo* getNetwork(int index);
    static bool isInitialized() { return wifiInitialized; }
    static void scanNetworks();

private:
    static void displayNetwork(int index);
    static void drawHeatmapSpectrum();
    static String generateRssiBar(int32_t rssi);
    static String getSecurityString(uint8_t authMode);

    static WiFiNetworkInfo networks[MAX_NETWORKS];
    static int networkCount;
    static int currentNetworkIndex;
    static unsigned long lastScanTime;
    static bool wifiInitialized;
    static int8_t channelRssi[NUM_WIFI_CHANNELS];
    static uint8_t channelNetworkCount[NUM_WIFI_CHANNELS];
    static unsigned long lastHeatmapScan;
};

#endif
