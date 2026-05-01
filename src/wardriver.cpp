// wardriver.cpp - Passive WiFi + BLE WiGLE logger for Cypherbox V2

#include "wardriver.h"
#include "display.h"
#include "input.h"
#include <SPI.h>

TinyGPSPlus Wardriver::gps;
HardwareSerial Wardriver::gpsSerial(1);
BLEScan* Wardriver::bleScan = nullptr;
WardriverStats Wardriver::stats;
char Wardriver::knownWifi[Wardriver::MAX_UNIQUE_WIFI][18];
char Wardriver::knownBle[Wardriver::MAX_UNIQUE_BLE][18];
uint16_t Wardriver::knownWifiCount = 0;
uint16_t Wardriver::knownBleCount = 0;
uint8_t Wardriver::page = 0;
bool Wardriver::loggingEnabled = true;
bool Wardriver::bleEnabled = true;
bool Wardriver::fastScan = false;
bool Wardriver::immediateScan = false;
bool Wardriver::stopRequested = false;
uint8_t Wardriver::rowsSinceFlush = 0;
uint32_t Wardriver::lastScanMs = 0;
uint32_t Wardriver::lastDisplayMs = 0;
uint32_t Wardriver::lastPageMs = 0;
uint32_t Wardriver::lastSummaryMs = 0;
uint32_t Wardriver::lastButtonMs = 0;
char Wardriver::serialBuffer[96];
uint8_t Wardriver::serialIndex = 0;

void Wardriver::run() {
    resetSession();
    if (!begin()) {
        delay(1800);
        return;
    }

    Display::displayInfo("Wardriver", "Passive WiFi+BLE", "Waiting for GPS", "SEL=exit");
    Serial.println(F("\n=== Cypherbox Wardriver ==="));
    Serial.println(F("Passive WiFi + BLE WiGLE v1.6 logger"));
    Serial.println(F("Commands: wardriver_status, wardriver_gps, wardriver_scan, wardriver_log on|off, wardriver_ble on|off, wardriver_fast on|off, wardriver_rotate, stop"));

    while (!stopRequested) {
        updateGps();
        WardriverFix fix = currentFix();
        handleButtons();
        processSerial();

        uint32_t interval = fastScan ? FAST_SCAN_INTERVAL_MS : NORMAL_SCAN_INTERVAL_MS;
        if (immediateScan || millis() - lastScanMs >= interval) {
            immediateScan = false;
            lastScanMs = millis();
            if (!loggingEnabled) {
                Serial.println(F("Wardriver logging paused"));
            } else if (freshFixOrWait(fix) && ensureLog(fix)) {
                scanWifi(fix);
                if (bleEnabled) {
                    scanBle(fix);
                }
                maybeWriteSummary(fix);
            }
        }

        render(fix);
        yield();
    }

    end();
    Display::displayInfo("Wardriver", "Stopped", String(stats.wifiRows) + " WiFi rows", String(stats.bleRows) + " BLE rows");
    delay(1500);
}

bool Wardriver::begin() {
    gpsSerial.begin(9600, SERIAL_8N1, GPS_RX, GPS_TX);

    SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
    if (!SD.begin(SD_CS)) {
        setError("SD init failed");
        Display::displayInfo("Wardriver", "SD init failed", "Check FAT32/card", "");
        return false;
    }

    WiFi.mode(WIFI_STA);
    WiFi.disconnect(true);
    delay(100);

    BLEDevice::init("CypherboxWardriver");
    bleScan = BLEDevice::getScan();
    if (bleScan) {
        bleScan->setActiveScan(false);
        bleScan->setInterval(160);
        bleScan->setWindow(80);
    }

    lastScanMs = millis() - NORMAL_SCAN_INTERVAL_MS;
    lastDisplayMs = 0;
    lastPageMs = millis();
    return true;
}

void Wardriver::end() {
    WiFi.scanDelete();
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    if (bleScan) {
        bleScan->clearResults();
    }
    BLEDevice::deinit();
    bleScan = nullptr;
}

void Wardriver::resetSession() {
    stats = WardriverStats();
    memset(knownWifi, 0, sizeof(knownWifi));
    memset(knownBle, 0, sizeof(knownBle));
    knownWifiCount = 0;
    knownBleCount = 0;
    page = 0;
    loggingEnabled = true;
    bleEnabled = true;
    fastScan = false;
    immediateScan = true;
    stopRequested = false;
    rowsSinceFlush = 0;
    serialIndex = 0;
    serialBuffer[0] = '\0';
}

void Wardriver::updateGps() {
    while (gpsSerial.available() > 0) {
        gps.encode(gpsSerial.read());
    }
}

WardriverFix Wardriver::currentFix() {
    WardriverFix fix;
    fix.valid = gps.location.isValid() && gps.hdop.isValid() && gps.satellites.isValid();
    fix.satellites = gps.satellites.isValid() ? gps.satellites.value() : 0;
    fix.ageMs = gps.location.isValid() ? gps.location.age() : UINT32_MAX;
    fix.fresh = fix.valid && fix.satellites >= MIN_FIX_SATELLITES && fix.ageMs <= GPS_FIX_STALE_MS;
    fix.latitude = gps.location.isValid() ? gps.location.lat() : 0.0;
    fix.longitude = gps.location.isValid() ? gps.location.lng() : 0.0;
    fix.altitudeMeters = gps.altitude.isValid() ? gps.altitude.meters() : 0.0;
    fix.hdop = gps.hdop.isValid() ? gps.hdop.hdop() : 99.9;
    fix.charsProcessed = gps.charsProcessed();
    formatGpsTimestamp(fix.timestamp, sizeof(fix.timestamp));
    formatFilenameStamp(fix.filenameStamp, sizeof(fix.filenameStamp));
    return fix;
}

bool Wardriver::isGpsDateTimeValid() {
    return gps.date.isValid() && gps.time.isValid() && gps.date.year() >= 2024;
}

void Wardriver::formatGpsTimestamp(char* out, size_t len) {
    if (isGpsDateTimeValid()) {
        snprintf(out, len, "%04d-%02d-%02d %02d:%02d:%02d",
                 gps.date.year(), gps.date.month(), gps.date.day(),
                 gps.time.hour(), gps.time.minute(), gps.time.second());
    } else {
        uint32_t seconds = millis() / 1000UL;
        snprintf(out, len, "1970-01-01 %02lu:%02lu:%02lu",
                 (seconds / 3600UL) % 24UL, (seconds / 60UL) % 60UL, seconds % 60UL);
    }
}

void Wardriver::formatFilenameStamp(char* out, size_t len) {
    if (isGpsDateTimeValid()) {
        snprintf(out, len, "%04d%02d%02d_%02d%02d%02d",
                 gps.date.year(), gps.date.month(), gps.date.day(),
                 gps.time.hour(), gps.time.minute(), gps.time.second());
    } else {
        snprintf(out, len, "BOOT_%lu", millis() / 1000UL);
    }
}

void Wardriver::handleButtons() {
    if (millis() - lastButtonMs < 180) return;

    if (Input::isButtonPressed(BUTTON_UP)) {
        page = (page + 4) % 5;
        lastPageMs = millis();
        lastButtonMs = millis();
    } else if (Input::isButtonPressed(BUTTON_DOWN)) {
        page = (page + 1) % 5;
        lastPageMs = millis();
        lastButtonMs = millis();
    } else if (Input::isButtonPressed(BUTTON_SELECT)) {
        stopRequested = true;
        lastButtonMs = millis();
    }
}

void Wardriver::processSerial() {
    while (Serial.available() > 0) {
        char c = Serial.read();
        if (c == '\r' || c == '\n') {
            if (serialIndex > 0) {
                serialBuffer[serialIndex] = '\0';
                handleSerialLine(String(serialBuffer));
                serialIndex = 0;
                serialBuffer[0] = '\0';
            }
        } else if (serialIndex < sizeof(serialBuffer) - 1) {
            serialBuffer[serialIndex++] = c;
        }
    }
}

void Wardriver::handleSerialLine(String line) {
    line.trim();
    String lower = line;
    lower.toLowerCase();
    WardriverFix fix = currentFix();

    if (lower == "stop") {
        stopRequested = true;
    } else if (lower == "wardriver_status") {
        printStatus(fix);
    } else if (lower == "wardriver_gps") {
        printGps(fix);
    } else if (lower == "wardriver_scan") {
        immediateScan = true;
        Serial.println(F("Wardriver scan queued"));
    } else if (lower == "wardriver_log on") {
        loggingEnabled = true;
        Serial.println(F("Wardriver logging enabled"));
    } else if (lower == "wardriver_log off") {
        loggingEnabled = false;
        Serial.println(F("Wardriver logging paused"));
    } else if (lower == "wardriver_ble on") {
        bleEnabled = true;
        Serial.println(F("Wardriver BLE logging enabled"));
    } else if (lower == "wardriver_ble off") {
        bleEnabled = false;
        Serial.println(F("Wardriver BLE logging disabled"));
    } else if (lower == "wardriver_fast on") {
        fastScan = true;
        Serial.println(F("Wardriver fast scan enabled"));
    } else if (lower == "wardriver_fast off") {
        fastScan = false;
        Serial.println(F("Wardriver fast scan disabled"));
    } else if (lower == "wardriver_rotate") {
        if (rotateLog(fix)) {
            stats.rotations++;
            Serial.print(F("Wardriver log rotated to "));
            Serial.println(stats.activeLog);
        } else {
            stats.sdErrors++;
            setError("Rotate failed");
        }
    } else {
        Serial.print(F("Wardriver unknown command: "));
        Serial.println(line);
    }
}

void Wardriver::printStatus(const WardriverFix& fix) {
    Serial.println(F("\n=== Wardriver Status ==="));
    Serial.printf("Logging: %s\n", loggingEnabled ? "ON" : "PAUSED");
    Serial.printf("BLE: %s\n", bleEnabled ? "ON" : "OFF");
    Serial.printf("Fast scan: %s\n", fastScan ? "ON" : "OFF");
    Serial.printf("GPS fresh: %s sat:%u hdop:%.1f age:%lus\n",
                  fix.fresh ? "YES" : "NO", fix.satellites, fix.hdop,
                  fix.ageMs == UINT32_MAX ? 0 : fix.ageMs / 1000UL);
    Serial.printf("Scans:%lu WiFi rows:%lu BLE rows:%lu\n", stats.scans, stats.wifiRows, stats.bleRows);
    Serial.printf("Unique WiFi:%lu Unique BLE:%lu SD errors:%lu\n", stats.uniqueWifi, stats.uniqueBle, stats.sdErrors);
    Serial.printf("Log: %s\n", stats.activeLog[0] ? stats.activeLog : "none");
    Serial.printf("Summary: %s\n", stats.activeSummary[0] ? stats.activeSummary : "none");
    Serial.printf("Last error: %s\n", stats.lastError[0] ? stats.lastError : "none");
}

void Wardriver::printGps(const WardriverFix& fix) {
    Serial.println(F("\n=== Wardriver GPS ==="));
    Serial.printf("Fresh:%s Valid:%s Sat:%u Age:%lu ms Chars:%lu\n",
                  fix.fresh ? "YES" : "NO", fix.valid ? "YES" : "NO",
                  fix.satellites, fix.ageMs, fix.charsProcessed);
    Serial.printf("Lat:%.6f Lng:%.6f Alt:%.1fm HDOP:%.1f\n",
                  fix.latitude, fix.longitude, fix.altitudeMeters, fix.hdop);
    Serial.printf("Timestamp:%s FileStamp:%s\n", fix.timestamp, fix.filenameStamp);
}

bool Wardriver::freshFixOrWait(const WardriverFix& fix) {
    if (fix.fresh) return true;
    stats.gpsWaits++;
    Serial.println(F("Wardriver waiting for fresh GPS fix"));
    Display::displayInfo("Wardriver", "Waiting for GPS", "Sat " + String(fix.satellites), "SEL=exit");
    return false;
}

bool Wardriver::ensureLog(const WardriverFix& fix) {
    if (stats.activeLog[0] == '\0') {
        return createLog(fix);
    }
    if (activeLogBytes() >= LOG_ROTATE_BYTES) {
        return rotateLog(fix);
    }
    return true;
}

bool Wardriver::createLog(const WardriverFix& fix) {
    snprintf(stats.activeLog, sizeof(stats.activeLog), "/wigle_%s.csv", fix.filenameStamp);
    snprintf(stats.activeSummary, sizeof(stats.activeSummary), "/summary_%s.txt", fix.filenameStamp);

    File file = SD.open(stats.activeLog, FILE_WRITE);
    if (!file) {
        setError("Log open failed");
        stats.activeLog[0] = '\0';
        stats.activeSummary[0] = '\0';
        return false;
    }

    writeHeader(file);
    bool ok = file.getWriteError() == 0;
    file.close();
    if (!ok) {
        setError("Header write failed");
    }
    return ok;
}

bool Wardriver::rotateLog(const WardriverFix& fix) {
    stats.activeLog[0] = '\0';
    stats.activeSummary[0] = '\0';
    rowsSinceFlush = 0;
    return createLog(fix);
}

uint32_t Wardriver::activeLogBytes() {
    if (stats.activeLog[0] == '\0' || !SD.exists(stats.activeLog)) return 0;
    File file = SD.open(stats.activeLog, FILE_READ);
    if (!file) return 0;
    uint32_t size = file.size();
    file.close();
    return size;
}

void Wardriver::writeHeader(File& file) {
    file.println(F("WigleWifi-1.6,appRelease=CypherboxWardriver,model=ESP32,release=2.0,device=Cypherbox,display=SSD1306,board=ESP32"));
    file.println(F("MAC,SSID,AuthMode,FirstSeen,Channel,Frequency,RSSI,CurrentLatitude,CurrentLongitude,AltitudeMeters,AccuracyMeters,RCOIs,MfgrId,Type"));
}

void Wardriver::scanWifi(const WardriverFix& fix) {
    int n = WiFi.scanNetworks(false, true);
    if (n < 0) {
        stats.sdErrors++;
        setError("WiFi scan failed");
        return;
    }

    stats.scans++;
    stats.wifiSeen += n;
    File file = SD.open(stats.activeLog, FILE_APPEND);
    if (!file) {
        stats.sdErrors++;
        setError("WiFi log open failed");
        WiFi.scanDelete();
        return;
    }

    for (int i = 0; i < n; i++) {
        if (!writeWifiRow(file, i, fix)) {
            stats.sdErrors++;
            setError("WiFi row failed");
            break;
        }
    }

    if (rowsSinceFlush >= LOG_FLUSH_ROWS) {
        file.flush();
        rowsSinceFlush = 0;
    }
    file.close();
    WiFi.scanDelete();
}

void Wardriver::scanBle(const WardriverFix& fix) {
    if (!bleScan) return;

    BLEScanResults* results = bleScan->start(BLE_SCAN_DURATION, false);
    if (!results) {
        stats.sdErrors++;
        setError("BLE scan failed");
        return;
    }

    int count = min(results->getCount(), MAX_BLE_DEVICES);
    stats.bleSeen += count;
    File file = SD.open(stats.activeLog, FILE_APPEND);
    if (!file) {
        stats.sdErrors++;
        setError("BLE log open failed");
        bleScan->clearResults();
        return;
    }

    for (int i = 0; i < count; i++) {
        BLEAdvertisedDevice device = results->getDevice(i);
        if (!writeBleRow(file, device, fix)) {
            stats.sdErrors++;
            setError("BLE row failed");
            break;
        }
    }

    if (rowsSinceFlush >= LOG_FLUSH_ROWS) {
        file.flush();
        rowsSinceFlush = 0;
    }
    file.close();
    bleScan->clearResults();
}

bool Wardriver::writeWifiRow(File& file, int index, const WardriverFix& fix) {
    String bssid = WiFi.BSSIDstr(index);
    String ssid = WiFi.SSID(index);
    int32_t channel = WiFi.channel(index);
    int32_t rssi = WiFi.RSSI(index);
    wifi_auth_mode_t auth = WiFi.encryptionType(index);

    if (noteUnique(knownWifi, knownWifiCount, MAX_UNIQUE_WIFI, bssid.c_str())) {
        stats.uniqueWifi++;
    }
    if (ssid.length() == 0) stats.hiddenWifi++;
    if (auth == WIFI_AUTH_OPEN) stats.openWifi++;
    else stats.securedWifi++;

    updateTop(stats.topWifi, bssid.c_str(), ssid.length() ? ssid.c_str() : "<hidden>", rssi, channel);

    file.print(csvEscape(bssid)); file.print(',');
    file.print(csvEscape(ssid.length() ? ssid : String("<hidden>"))); file.print(',');
    file.print(csvEscape(authMode(auth))); file.print(',');
    file.print(csvEscape(fix.timestamp)); file.print(',');
    file.print(channel); file.print(',');
    file.print(channelFrequency(channel)); file.print(',');
    file.print(rssi); file.print(',');
    file.print(fix.latitude, 6); file.print(',');
    file.print(fix.longitude, 6); file.print(',');
    file.print(fix.altitudeMeters, 1); file.print(',');
    file.print(fix.hdop, 1); file.print(',');
    file.print(",,");
    file.println(F("WIFI"));

    stats.wifiRows++;
    rowsSinceFlush++;
    return file.getWriteError() == 0;
}

bool Wardriver::writeBleRow(File& file, BLEAdvertisedDevice& device, const WardriverFix& fix) {
    String address = device.getAddress().toString().c_str();
    String name = device.haveName() ? String(device.getName().c_str()) : String("<no name>");
    int32_t rssi = device.getRSSI();

    if (noteUnique(knownBle, knownBleCount, MAX_UNIQUE_BLE, address.c_str())) {
        stats.uniqueBle++;
    }
    updateTop(stats.topBle, address.c_str(), name.c_str(), rssi, 0);

    file.print(csvEscape(address)); file.print(',');
    file.print(csvEscape(name)); file.print(',');
    file.print(csvEscape(device.isConnectable() ? "BLE_CONNECTABLE" : "BLE_ADVERTISING")); file.print(',');
    file.print(csvEscape(fix.timestamp)); file.print(',');
    file.print('0'); file.print(',');
    file.print(',');
    file.print(rssi); file.print(',');
    file.print(fix.latitude, 6); file.print(',');
    file.print(fix.longitude, 6); file.print(',');
    file.print(fix.altitudeMeters, 1); file.print(',');
    file.print(fix.hdop, 1); file.print(',');
    file.print(",,");
    file.println(F("BLE"));

    stats.bleRows++;
    rowsSinceFlush++;
    return file.getWriteError() == 0;
}

void Wardriver::maybeWriteSummary(const WardriverFix& fix) {
    if (stats.activeSummary[0] == '\0') return;
    if (millis() - lastSummaryMs < SUMMARY_INTERVAL_MS) return;
    if (!writeSummary(fix)) {
        stats.sdErrors++;
        setError("Summary failed");
    }
    lastSummaryMs = millis();
}

bool Wardriver::writeSummary(const WardriverFix& fix) {
    File file = SD.open(stats.activeSummary, FILE_WRITE);
    if (!file) return false;

    file.println(F("Cypherbox Wardriver Session"));
    file.print(F("Updated: ")); file.println(fix.timestamp);
    file.print(F("Log: ")); file.println(stats.activeLog);
    file.print(F("Logging: ")); file.println(loggingEnabled ? F("on") : F("paused"));
    file.print(F("BLE: ")); file.println(bleEnabled ? F("on") : F("off"));
    file.print(F("Fast scan: ")); file.println(fastScan ? F("on") : F("off"));
    file.print(F("Scans: ")); file.println(stats.scans);
    file.print(F("WiFi rows: ")); file.println(stats.wifiRows);
    file.print(F("BLE rows: ")); file.println(stats.bleRows);
    file.print(F("Unique WiFi BSSIDs: ")); file.println(stats.uniqueWifi);
    file.print(F("Unique BLE addresses: ")); file.println(stats.uniqueBle);
    file.print(F("Open WiFi: ")); file.println(stats.openWifi);
    file.print(F("Secured WiFi: ")); file.println(stats.securedWifi);
    file.print(F("Hidden WiFi: ")); file.println(stats.hiddenWifi);
    file.print(F("GPS waits: ")); file.println(stats.gpsWaits);
    file.print(F("SD errors: ")); file.println(stats.sdErrors);
    file.print(F("Last error: ")); file.println(stats.lastError[0] ? stats.lastError : "none");

    bool ok = file.getWriteError() == 0;
    file.close();
    return ok;
}

void Wardriver::render(const WardriverFix& fix) {
    if (millis() - lastPageMs >= DISPLAY_PAGE_MS) {
        page = (page + 1) % 5;
        lastPageMs = millis();
    }
    if (millis() - lastDisplayMs < 350) return;
    lastDisplayMs = millis();

    switch (page) {
        case 0: renderGps(fix); break;
        case 1: renderWifi(); break;
        case 2: renderBle(); break;
        case 3: renderTop(); break;
        default: renderStorage(); break;
    }
}

void Wardriver::renderGps(const WardriverFix& fix) {
    Display::getOled().clearDisplay();
    Display::getOled().setTextSize(1);
    Display::getOled().setTextColor(SSD1306_WHITE);
    Display::getOled().setCursor(0, 0);
    Display::getOled().println(loggingEnabled ? "Wardriver GPS LOG" : "Wardriver GPS PAUSE");
    Display::getOled().drawLine(0, 10, SCREEN_WIDTH, 10, SSD1306_WHITE);
    Display::getOled().setCursor(0, 14);
    Display::getOled().print("Fix: ");
    Display::getOled().print(fix.fresh ? "fresh" : "waiting");
    Display::getOled().print(" Sat:");
    Display::getOled().println(fix.satellites);
    Display::getOled().print("Lat ");
    Display::getOled().println(fix.latitude, 5);
    Display::getOled().print("Lng ");
    Display::getOled().println(fix.longitude, 5);
    Display::getOled().print("HDOP ");
    Display::getOled().print(fix.hdop, 1);
    Display::getOled().print(" Age ");
    Display::getOled().print(fix.ageMs == UINT32_MAX ? 0 : fix.ageMs / 1000);
    Display::getOled().println("s");
    Display::getOled().display();
}

void Wardriver::renderWifi() {
    Display::displayInfo("Wardriver WiFi",
                         "Rows " + String(stats.wifiRows) + " Unique " + String(stats.uniqueWifi),
                         "Open " + String(stats.openWifi) + " Sec " + String(stats.securedWifi),
                         "Hidden " + String(stats.hiddenWifi));
}

void Wardriver::renderBle() {
    Display::displayInfo("Wardriver BLE",
                         bleEnabled ? "BLE logging ON" : "BLE logging OFF",
                         "Rows " + String(stats.bleRows) + " Unique " + String(stats.uniqueBle),
                         "Seen " + String(stats.bleSeen));
}

void Wardriver::renderTop() {
    Display::getOled().clearDisplay();
    Display::getOled().setTextSize(1);
    Display::getOled().setTextColor(SSD1306_WHITE);
    Display::getOled().setCursor(0, 0);
    Display::getOled().println("Top Signals");
    Display::getOled().drawLine(0, 10, SCREEN_WIDTH, 10, SSD1306_WHITE);
    Display::getOled().setCursor(0, 14);
    for (uint8_t i = 0; i < 2; i++) {
        Display::getOled().print("W ");
        if (stats.topWifi[i].id[0]) {
            Display::getOled().print(stats.topWifi[i].rssi);
            Display::getOled().print(" ");
            printClipped(stats.topWifi[i].label, 12);
        } else {
            Display::getOled().print("--");
        }
        Display::getOled().println();
    }
    for (uint8_t i = 0; i < 2; i++) {
        Display::getOled().print("B ");
        if (stats.topBle[i].id[0]) {
            Display::getOled().print(stats.topBle[i].rssi);
            Display::getOled().print(" ");
            printClipped(stats.topBle[i].label, 12);
        } else {
            Display::getOled().print("--");
        }
        Display::getOled().println();
    }
    Display::getOled().display();
}

void Wardriver::renderStorage() {
    Display::displayInfo("Wardriver SD",
                         stats.activeLog[0] ? String(stats.activeLog).substring(0, 21) : "No log yet",
                         "Bytes " + String(activeLogBytes()),
                         stats.lastError[0] ? String(stats.lastError).substring(0, 21) : "No errors");
}

void Wardriver::setError(const char* message) {
    strncpy(stats.lastError, message, sizeof(stats.lastError) - 1);
    stats.lastError[sizeof(stats.lastError) - 1] = '\0';
    Serial.print(F("Wardriver error: "));
    Serial.println(stats.lastError);
}

bool Wardriver::noteUnique(char list[][18], uint16_t& count, uint16_t maxCount, const char* id) {
    for (uint16_t i = 0; i < count; i++) {
        if (strncmp(list[i], id, 18) == 0) return false;
    }
    if (count >= maxCount) return false;
    strncpy(list[count], id, 17);
    list[count][17] = '\0';
    count++;
    return true;
}

void Wardriver::updateTop(WardriverTopSignal top[], const char* id, const char* label, int32_t rssi, int32_t channel) {
    for (uint8_t i = 0; i < TOP_SIGNAL_COUNT; i++) {
        if (strncmp(top[i].id, id, sizeof(top[i].id)) == 0) {
            strncpy(top[i].label, label, sizeof(top[i].label) - 1);
            top[i].rssi = rssi;
            top[i].channel = channel;
            return;
        }
    }

    for (uint8_t i = 0; i < TOP_SIGNAL_COUNT; i++) {
        if (rssi > top[i].rssi) {
            for (int8_t j = TOP_SIGNAL_COUNT - 1; j > i; j--) {
                top[j] = top[j - 1];
            }
            strncpy(top[i].id, id, sizeof(top[i].id) - 1);
            strncpy(top[i].label, label, sizeof(top[i].label) - 1);
            top[i].rssi = rssi;
            top[i].channel = channel;
            return;
        }
    }
}

String Wardriver::csvEscape(const String& value) {
    String escaped = "\"";
    for (uint16_t i = 0; i < value.length(); i++) {
        char c = value.charAt(i);
        if (c == '"') escaped += "\"\"";
        else if (c >= 32 && c != 127) escaped += c;
    }
    escaped += "\"";
    return escaped;
}

const char* Wardriver::authMode(wifi_auth_mode_t auth) {
    switch (auth) {
        case WIFI_AUTH_OPEN: return "OPEN";
        case WIFI_AUTH_WEP: return "WEP";
        case WIFI_AUTH_WPA_PSK: return "WPA_PSK";
        case WIFI_AUTH_WPA2_PSK: return "WPA2_PSK";
        case WIFI_AUTH_WPA_WPA2_PSK: return "WPA_WPA2_PSK";
        case WIFI_AUTH_WPA2_ENTERPRISE: return "WPA2_ENTERPRISE";
        case WIFI_AUTH_WPA3_PSK: return "WPA3_PSK";
        case WIFI_AUTH_WPA2_WPA3_PSK: return "WPA2_WPA3_PSK";
        default: return "UNKNOWN";
    }
}

int32_t Wardriver::channelFrequency(int32_t channel) {
    if (channel >= 1 && channel <= 13) return 2407 + channel * 5;
    if (channel == 14) return 2484;
    if (channel >= 32 && channel <= 177) return 5000 + channel * 5;
    return 0;
}

void Wardriver::printClipped(const char* text, uint8_t maxChars) {
    for (uint8_t i = 0; text[i] != '\0' && i < maxChars; i++) {
        Display::getOled().print(text[i]);
    }
}
