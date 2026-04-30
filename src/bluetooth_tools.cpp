// bluetooth_tools.cpp - Bluetooth helper modes for Cypherbox V2

#include "bluetooth_tools.h"
#include "display.h"
#include "input.h"
#include "ble_scanner.h"
#include <BluetoothSerial.h>
#include <WiFi.h>

static BluetoothSerial CypherboxBT;
bool BluetoothTools::serialRunning = false;
static String btSsid = "";
static String btPassword = "";

void BluetoothTools::startSerial() {
    if (serialRunning) {
        Display::displayInfo("BT Serial", "Already running", "cypherboxBT", "");
        return;
    }
    if (CypherboxBT.begin("cypherboxBT")) {
        serialRunning = true;
        Serial.println("Bluetooth Serial started as cypherboxBT");
        Display::displayInfo("BT Serial", "Started", "cypherboxBT", "Pair then HELP");
    } else {
        Serial.println("Bluetooth Serial start failed");
        Display::displayInfo("BT Serial", "Start failed", "", "");
    }
    delay(1600);
}

void BluetoothTools::handleSerialCommand(const String& commandRaw) {
    String command = commandRaw;
    command.trim();
    command.toUpperCase();
    Serial.println("BT command: " + command);

    if (command == "HELP") {
        CypherboxBT.println("HELP, WIFI <ssid>, PASS <password>, START_WIFI, STOP_WIFI, SCAN_BLE, STATUS, STOP_BT");
    } else if (commandRaw.startsWith("WIFI ")) {
        btSsid = commandRaw.substring(5);
        btSsid.trim();
        CypherboxBT.println("SSID set: " + btSsid);
    } else if (commandRaw.startsWith("PASS ")) {
        btPassword = commandRaw.substring(5);
        btPassword.trim();
        CypherboxBT.println("Password set");
    } else if (command == "START_WIFI") {
        WiFi.mode(WIFI_STA);
        WiFi.begin(btSsid.c_str(), btPassword.c_str());
        CypherboxBT.println("Connecting...");
    } else if (command == "STOP_WIFI") {
        WiFi.disconnect(true);
        CypherboxBT.println("WiFi stopped");
    } else if (command == "SCAN_BLE") {
        BLEScanner::init();
        BLEScanner::performScan();
        CypherboxBT.printf("BLE devices: %d\n", BLEScanner::getDeviceCount());
    } else if (command == "STATUS") {
        CypherboxBT.printf("Heap:%lu WiFi:%d BLE:%d\n", ESP.getFreeHeap(), WiFi.status(), BLEScanner::getDeviceCount());
    } else if (command == "STOP_BT") {
        CypherboxBT.println("Stopping BT");
        stop();
    } else {
        CypherboxBT.println("Unknown command. Type HELP.");
    }
}

void BluetoothTools::runSerialBridge() {
    startSerial();
    Display::displayInfo("BT Serial", "Pair cypherboxBT", "Type HELP", "SEL exits");
    delay(250);
    while (serialRunning) {
        if (CypherboxBT.available()) {
            String command = CypherboxBT.readStringUntil('\n');
            handleSerialCommand(command);
            Display::displayInfo("BT Serial", "Cmd received", command.substring(0, 21), "SEL exits");
        }
        if (Input::isButtonPressed(BUTTON_SELECT)) {
            delay(200);
            stop();
            return;
        }
        yield();
    }
}

void BluetoothTools::runHidSafeTest() {
    startSerial();
    Display::displayInfo("BT HID Test", "Safe test only", "SEL sends text", "UP exits");
    Serial.println("BT HID safe test uses Bluetooth Serial text only; no keystroke injection.");
    delay(250);
    while (serialRunning) {
        if (Input::isButtonPressed(BUTTON_SELECT)) {
            CypherboxBT.println("Cypherbox HID safe test: hello from authorized lab mode.");
            Serial.println("BT HID safe test string sent");
            Display::displayInfo("BT HID Test", "Test string sent", "No keystrokes", "UP exits");
            delay(500);
        }
        if (Input::isButtonPressed(BUTTON_UP)) {
            delay(200);
            stop();
            return;
        }
        yield();
    }
}

void BluetoothTools::stop() {
    if (serialRunning) {
        CypherboxBT.end();
        serialRunning = false;
    }
    Serial.println("Bluetooth stopped");
}
