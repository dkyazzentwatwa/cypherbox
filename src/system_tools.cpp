// system_tools.cpp - WiFi/AP/SD utility tools for Cypherbox V2

#include "system_tools.h"
#include "display.h"
#include "input.h"
#include "packet_monitor.h"
#include "bluetooth_tools.h"
#include "ble_scanner.h"
#include "cypherbox_webserver.h"
#include <WiFi.h>
#include <SPI.h>
#include <SD.h>
#include "../config.h"

bool SystemTools::initSd() {
    SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
    if (!SD.begin(SD_CS)) {
        Serial.println("SD init failed");
        Display::displayInfo("SD Card", "Init failed", "Check FAT32/card", "");
        return false;
    }
    Serial.printf("SD ready: %llu MB\n", SD.cardSize() / (1024ULL * 1024ULL));
    return true;
}

void SystemTools::listSdFiles() {
    if (!initSd()) return;
    File root = SD.open("/");
    Serial.println("\n=== SD Files ===");
    while (true) {
        File entry = root.openNextFile();
        if (!entry) break;
        Serial.printf("%s%s %u bytes\n", entry.isDirectory() ? "[DIR] " : "", entry.name(), (unsigned)entry.size());
        entry.close();
    }
    root.close();
}

void SystemTools::browseSdFiles() {
    if (!initSd()) return;
    char names[40][64] = {0};
    int count = 0;
    File root = SD.open("/");
    while (count < 40) {
        File entry = root.openNextFile();
        if (!entry) break;
        strncpy(names[count], entry.name(), sizeof(names[count]) - 1);
        count++;
        entry.close();
    }
    root.close();
    if (count == 0) {
        Display::displayInfo("SD Files", "No files found", "", "");
        delay(1500);
        return;
    }
    int selected = 0;
    delay(250);
    while (true) {
        Display::displayInfo("SD " + String(selected + 1) + "/" + String(count),
                             String(names[selected]).substring(0, 21),
                             "UP/DN nav",
                             "SEL exit");
        if (Input::isButtonPressed(BUTTON_UP)) {
            selected = (selected - 1 + count) % count;
            delay(180);
        }
        if (Input::isButtonPressed(BUTTON_DOWN)) {
            selected = (selected + 1) % count;
            delay(180);
        }
        if (Input::isButtonPressed(BUTTON_SELECT)) {
            delay(200);
            return;
        }
        yield();
    }
}

void SystemTools::readSdFile(const String& pathArg) {
    if (!initSd()) return;
    String path = pathArg;
    if (path.length() == 0) {
        Display::displayInfo("SD Read", "Use serial:", "sd_read <file>", "");
        listSdFiles();
        delay(1800);
        return;
    }
    if (!path.startsWith("/")) path = "/" + path;
    File file = SD.open(path, FILE_READ);
    if (!file) {
        Serial.println("Could not open " + path);
        Display::displayInfo("SD Read", "Open failed", path.substring(0, 21), "");
        return;
    }
    Serial.println("\n=== " + path + " ===");
    String preview = "";
    int shown = 0;
    while (file.available() && shown < 512) {
        char c = file.read();
        Serial.write(c);
        if (preview.length() < 80 && c >= 32 && c < 127) preview += c;
        shown++;
    }
    Serial.println("\n=== EOF/PREVIEW LIMIT ===");
    file.close();
    Display::displayInfo("SD Read",
                         path.substring(0, 21),
                         preview.substring(0, 21),
                         "Serial has preview");
    delay(2200);
}

void SystemTools::deleteSdFile(const String& pathArg, bool confirmed) {
    if (!initSd()) return;
    String path = pathArg;
    if (!path.startsWith("/")) path = "/" + path;
    if (!confirmed) {
        Serial.println("Refusing delete without confirmation. Use: sd_delete <file> confirm");
        Display::displayInfo("SD Delete", "Needs confirm", "sd_delete file confirm", "");
        return;
    }
    if (SD.remove(path)) {
        Serial.println("Deleted " + path);
        Display::displayInfo("SD Delete", "Deleted", path.substring(0, 21), "");
    } else {
        Serial.println("Delete failed " + path);
        Display::displayInfo("SD Delete", "Failed", path.substring(0, 21), "");
    }
    delay(1400);
}

void SystemTools::startAp() {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID, AP_PASSWORD, AP_CHANNEL, AP_HIDDEN, AP_MAX_CONNECTIONS);
    IPAddress ip = WiFi.softAPIP();
    Serial.printf("AP started SSID=%s IP=%s\n", AP_SSID, ip.toString().c_str());
    Display::displayInfo("AP Started", AP_SSID, ip.toString(), "Pass " + String(AP_PASSWORD));
    delay(1800);
}

void SystemTools::stopAp() {
    if (StarbeamWebServer::isRunning()) StarbeamWebServer::stop();
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_OFF);
    Serial.println("AP stopped");
    Display::displayInfo("AP", "Stopped", "", "");
    delay(1200);
}

void SystemTools::joinWiFi(const String& ssid, const String& password) {
    if (ssid.length() == 0) {
        Serial.println("Usage: wifi_join <ssid> <password>");
        Display::displayInfo("WiFi Join", "Use serial:", "wifi_join ssid pass", "");
        delay(1800);
        return;
    }
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), password.c_str());
    Display::displayInfo("WiFi Join", ssid.substring(0, 21), "Connecting...", "");
    for (int i = 0; i < 20 && WiFi.status() != WL_CONNECTED; i++) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("WiFi connected: " + WiFi.localIP().toString());
        Display::displayInfo("WiFi Connected", ssid.substring(0, 21), WiFi.localIP().toString(), "");
    } else {
        Serial.println("WiFi join failed");
        Display::displayInfo("WiFi Join", "Failed", ssid.substring(0, 21), "");
    }
    delay(1800);
}

void SystemTools::disconnectWiFi() {
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    Serial.println("WiFi disconnected");
}

void SystemTools::stopAll() {
    PacketMonitor::stop();
    if (StarbeamWebServer::isRunning()) StarbeamWebServer::stop();
    BluetoothTools::stop();
    BLEScanner::deinit();
    WiFi.softAPdisconnect(true);
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    Serial.println("All operations stopped");
    Display::displayInfo("Stop All", "Operations stopped", "", "");
    delay(1200);
}

void SystemTools::disabledLabMode(const String& label) {
    Serial.println(label + " disabled: attack/lab mode removed from this build");
    Display::displayInfo("Lab Mode Removed",
                         label.substring(0, 21),
                         "Excluded in V2",
                         "Non-attack build");
    delay(1800);
}
