// wifi_scanner.cpp - WiFi Scanner Module Implementation for Cypherbox V2

#include "wifi_scanner.h"
#include "input.h"
#include "terminal.h"

WiFiNetworkInfo WiFiScanner::networks[MAX_NETWORKS];
int WiFiScanner::networkCount = 0;
int WiFiScanner::currentNetworkIndex = 0;
unsigned long WiFiScanner::lastScanTime = 0;
bool WiFiScanner::wifiInitialized = false;
int8_t WiFiScanner::channelRssi[NUM_WIFI_CHANNELS];
uint8_t WiFiScanner::channelNetworkCount[NUM_WIFI_CHANNELS];
unsigned long WiFiScanner::lastHeatmapScan = 0;

void WiFiScanner::init() {
    if (wifiInitialized) return;
    Serial.println("Initializing WiFi for scanning...");
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);
    wifiInitialized = true;
    Serial.println("WiFi initialized for scanning");
}

void WiFiScanner::deinit() {
    if (!wifiInitialized) return;
    Serial.println("Deinitializing WiFi...");
    WiFi.disconnect();
    WiFi.mode(WIFI_OFF);
    esp_wifi_stop();
    esp_wifi_deinit();
    wifiInitialized = false;
}

const WiFiNetworkInfo* WiFiScanner::getNetwork(int index) {
    if (index >= 0 && index < networkCount) return &networks[index];
    return nullptr;
}

void WiFiScanner::scanNetworks() {
    int n = WiFi.scanNetworks(false, true);
    networkCount = min(n, MAX_NETWORKS);
    
    for (int i = 0; i < networkCount; i++) {
        networks[i].ssid = WiFi.SSID(i);
        networks[i].rssi = WiFi.RSSI(i);
        networks[i].channel = WiFi.channel(i);
        networks[i].authMode = WiFi.encryptionType(i);
        networks[i].bssid = WiFi.BSSIDstr(i);
    }
    
    lastScanTime = millis();
    Serial.printf("WiFi Scan: Found %d networks\n", networkCount);
}

void WiFiScanner::runScanner() {
    init();
    Display::displayInfo("WIFI SCANNER", "Scanning...", "", "");
    scanNetworks();
    currentNetworkIndex = 0;
    Serial.printf("WiFi Scanner: Found %d networks\n", networkCount);
    displayNetwork(currentNetworkIndex);

    while (true) {
        if (millis() - lastScanTime > WIFI_SCAN_INTERVAL) {
            scanNetworks();
            if (currentNetworkIndex >= networkCount && networkCount > 0) {
                currentNetworkIndex = networkCount - 1;
            }
            displayNetwork(currentNetworkIndex);
        }
        if (Input::isButtonPressed(BUTTON_UP)) {
            currentNetworkIndex = (currentNetworkIndex - 1 + networkCount) % max(networkCount, 1);
            displayNetwork(currentNetworkIndex);
            delay(200);
        }
        if (Input::isButtonPressed(BUTTON_DOWN)) {
            currentNetworkIndex = (currentNetworkIndex + 1) % max(networkCount, 1);
            displayNetwork(currentNetworkIndex);
            delay(200);
        }
        if (Input::isButtonPressed(BUTTON_SELECT)) {
            delay(200);
            return;
        }
    }
}

void WiFiScanner::runHeatmap() {
    init();
    Display::displayInfo("WIFI HEATMAP", "Scanning channels...", "", "");
    delay(1000);
    drawHeatmapSpectrum();

    while (true) {
        if (millis() - lastHeatmapScan > HEATMAP_SCAN_INTERVAL) {
            drawHeatmapSpectrum();
        }
        if (Input::isButtonPressed(BUTTON_SELECT)) {
            delay(200);
            return;
        }
    }
}

void WiFiScanner::drawHeatmapSpectrum() {
    Display::getOled().clearDisplay();
    Display::getOled().setTextSize(1);
    Display::getOled().setTextColor(SSD1306_WHITE);
    Display::getOled().setCursor(0, 0);
    Display::getOled().println("WiFi Channel Heatmap");

    memset(channelRssi, -100, sizeof(channelRssi));
    memset(channelNetworkCount, 0, sizeof(channelNetworkCount));

    for (int ch = 1; ch <= NUM_WIFI_CHANNELS; ch++) {
        esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE);
        delay(50);
        int16_t rssi = WiFi.RSSI();
        channelRssi[ch - 1] = rssi;
    }

    for (int ch = 1; ch <= NUM_WIFI_CHANNELS; ch++) {
        int barHeight = map(channelRssi[ch - 1], -100, -30, 0, 40);
        barHeight = constrain(barHeight, 0, 40);
        int x = (ch - 1) * 9 + 2;
        Display::getOled().fillRect(x, 63 - barHeight, 7, barHeight, SSD1306_WHITE);
    }
    Display::getOled().display();
    lastHeatmapScan = millis();
}

void WiFiScanner::displayNetwork(int index) {
    if (index < 0 || index >= networkCount) return;
    WiFiNetworkInfo& net = networks[index];
    String rssiBar = generateRssiBar(net.rssi);
    String info = String(net.channel) + " | " + rssiBar + " | " + getSecurityString(net.authMode);
    Display::displayInfo("WiFi: " + String(index + 1) + "/" + String(networkCount),
                         net.ssid.length() > 0 ? net.ssid : "(hidden)",
                         info,
                         "[UP/DN]=nav [SEL]=exit");
}

void WiFiScanner::listAll() {
    for (int i = 0; i < networkCount; i++) {
        WiFiNetworkInfo& net = networks[i];
        Serial.printf("[%d] %s (%s) ch:%d rssi:%d %s\n",
            i, net.ssid.c_str(), net.bssid.c_str(), net.channel, net.rssi,
            getSecurityString(net.authMode).c_str());
    }
}

String WiFiScanner::generateRssiBar(int32_t rssi) {
    int bars = map(rssi, -100, -30, 0, 4);
    bars = constrain(bars, 0, 4);
    String bar = "";
    for (int i = 0; i < 4; i++) bar += (i < bars) ? "#" : "-";
    return bar;
}

String WiFiScanner::getSecurityString(uint8_t authMode) {
    switch (authMode) {
        case 0: return "OPEN";
        case 1: return "WEP";
        case 2: return "WPA";
        case 3: return "WPA2";
        case 4: return "WPA/WPA2";
        case 5: return "WPA2-EAP";
        case 6: return "WPA3";
        case 7: return "WPA2/3";
        default: return "UNKNOWN";
    }
}
