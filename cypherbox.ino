// Cypherbox V2 - ESP32 Cybersecurity Toolkit
// Modular architecture with starbeam_v2 features (minus radio modules)

#include <Wire.h>
#include <SPI.h>
#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_bt.h"
#include "nvs_flash.h"
#include "driver/gpio.h"

// Cypherbox Original Libraries
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <U8g2_for_Adafruit_GFX.h>
#include <WiFi.h>
#include <SPI.h>
#include <TinyGPSPlus.h>
#include <HardwareSerial.h>
#include <time.h>
#include <RTClib.h>
#include <SD.h>
#include <Adafruit_NeoPixel.h>

// Cypherbox V2 Modules
#include "config.h"
#include "types.h"
#include "src/display.h"
#include "src/input.h"
#include "src/terminal.h"
#include "src/wifi_scanner.h"
#include "src/ble_scanner.h"
#include "src/Buffer.h"
#include "src/cypherbox_webserver.h"
#include "src/rfid_tools.h"
#include "src/packet_monitor.h"
#include "src/system_tools.h"
#include "src/bluetooth_tools.h"
#include "src/wifi_attack.h"
#include "src/captive_portal.h"

// ============================================================================
// Global State Variables
// ============================================================================

AppState currentState = STATE_MENU;
MenuItem selectedMenuItem = WIFI_SCAN;
int firstVisibleMenuItem = 0;

// Button mode flag (select pressed to enter mode)
bool enterMode = false;

// Neopixel
Adafruit_NeoPixel pixels(1, LED_PIN, NEO_GRB + NEO_KHZ800);

// GPS
TinyGPSPlus gps;
HardwareSerial GPSSerial(1);

// SD Card
bool useSD = false;
bool sdInitialized = false;

// ============================================================================
// Helper Functions
// ============================================================================

void nonBlockingDelay(unsigned long ms) {
    unsigned long start = millis();
    while (millis() - start < ms) {
        yield();
    }
}

// ============================================================================
// Cypherbox Original Feature Implementations
// ============================================================================

void runWardriver() {
    Display::displayInfo("WARDRIVER", "Starting...", "", "");
    Serial.println("Wardriver starting...");

    // Init GPS
    GPSSerial.begin(9600, SERIAL_8N1, GPS_RX, GPS_TX);
    
    // Init SD
    if (!SD.begin(SD_CS)) {
        Serial.println("SD Card init failed!");
        Display::displayInfo("WARDRIVER", "SD Card", "Init Failed!", "");
        delay(2000);
        return;
    }
    sdInitialized = true;

    File logFile = SD.open("/wardrive.csv", FILE_APPEND);
    if (!logFile) {
        logFile = SD.open("/wardrive.csv", FILE_WRITE);
        if (logFile) {
            logFile.println("Timestamp,Latitude,Longitude,Alt_ft,Speed_mph,SSID,BSSID,RSSI,Channel,AuthMode");
            logFile.close();
        }
    }

    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);

    unsigned long lastSave = 0;
    int networksFound = 0;

    Display::displayInfo("WARDRIVER", "Scanning...", "", "");

    while (true) {
        // Read GPS
        while (GPSSerial.available() > 0) {
            gps.encode(GPSSerial.read());
        }

        // Periodic WiFi scan
        int n = WiFi.scanNetworks();
        if (n > 0) {
            networksFound += n;
            for (int i = 0; i < n; i++) {
                File f = SD.open("/wardrive.csv", FILE_APPEND);
                if (f) {
                    f.print(gps.date.value());
                    f.print(",");
                    f.print(gps.location.lat(), 6);
                    f.print(",");
                    f.print(gps.location.lng(), 6);
                    f.print(",");
                    f.print(gps.altitude.feet());
                    f.print(",");
                    f.print(gps.speed.mph());
                    f.print(",");
                    f.print(WiFi.SSID(i));
                    f.print(",");
                    f.print(WiFi.BSSIDstr(i));
                    f.print(",");
                    f.print(WiFi.RSSI(i));
                    f.print(",");
                    f.print(WiFi.channel(i));
                    f.print(",");
                    f.println(WiFi.encryptionType(i));
                    f.close();
                }
            }
            WiFi.scanDelete();
        }

        // Update display every 2 seconds
        if (millis() - lastSave > 2000) {
            char buf[64];
            snprintf(buf, sizeof(buf), "GPS:%.4f,%.4f", gps.location.lat(), gps.location.lng());
            Display::displayInfo("WARDRIVER",
                                String(networksFound) + " nets logged",
                                buf,
                                "[SEL]=exit");
            lastSave = millis();
        }

        if (Input::isButtonPressed(BUTTON_SELECT)) {
            delay(200);
            WiFi.mode(WIFI_OFF);
            Display::displayInfo("WARDRIVER", "Stopped", String(networksFound) + " networks", "");
            delay(1500);
            return;
        }
        yield();
    }
}

void runPartyLight() {
    pixels.begin();
    pixels.setPixelColor(0, pixels.Color(0, 150, 0));
    pixels.show();
    Serial.println("Party light on");
    while (true) {
        // Cycle through colors
        for (int r = 0; r <= 255; r += 51) {
            for (int g = 0; g <= 255; g += 51) {
                for (int b = 0; b <= 255; b += 51) {
                    pixels.setPixelColor(0, pixels.Color(r, g, b));
                    pixels.show();
                    delay(50);
                    if (Input::isButtonPressed(BUTTON_SELECT)) {
                        delay(200);
                        pixels.setPixelColor(0, 0);
                        pixels.show();
                        return;
                    }
                }
            }
        }
    }
}

void runLightOff() {
    pixels.begin();
    pixels.setPixelColor(0, 0);
    pixels.show();
    Display::displayInfo("LIGHTS", "Off", "", "");
    delay(1000);
}

// ============================================================================
// Menu Execution
// ============================================================================

void executeSelectedMenuItem() {
    Serial.printf("Executing: %d\n", selectedMenuItem);
    String serialCommand = Terminal::isCommandFromSerial() ? Terminal::getCommandName() : "";

    switch (selectedMenuItem) {
        // === Cypherbox Original ===
        case PACKET_MON:
            currentState = STATE_PACKET_MON;
            PacketMonitor::runMonitor(false);
            break;

        case WIFI_SNIFF:
            currentState = STATE_WIFI_SNIFFER;
            PacketMonitor::runMonitor(true);
            break;

        case AP_SCAN:
            currentState = STATE_AP_SCAN;
            WiFiScanner::runScanner();
            WiFiScanner::deinit();
            break;

        case AP_JOIN:
            currentState = STATE_AP_JOIN;
            if (serialCommand == "wifi_join") {
                SystemTools::joinWiFi(Terminal::getArg(1), Terminal::getArgsAfter(2));
            } else {
                Display::displayInfo("WiFi Join", "Use serial:", "wifi_join ssid pass", "");
                delay(1800);
            }
            break;

        case AP_CREATE:
            currentState = STATE_AP_CREATE;
            SystemTools::startAp();
            break;

        case STOP_AP:
            currentState = STATE_STOP_AP;
            SystemTools::stopAp();
            break;

        case STOP_SERVER:
            currentState = STATE_STOP_SERVER;
            if (StarbeamWebServer::isRunning()) StarbeamWebServer::stop();
            Display::displayInfo("Web Server", "Stopped", "", "");
            delay(1200);
            break;

        case BT_SCAN:
            currentState = STATE_BT_SCAN;
            BLEScanner::runScanner();
            BLEScanner::deinit();
            break;

        case BT_CREATE:
            currentState = STATE_BT_SERIAL;
            BluetoothTools::startSerial();
            break;

        case BT_SERIAL_CMD:
            currentState = STATE_BT_SERIAL;
            BluetoothTools::runSerialBridge();
            break;

        case BT_HID:
            currentState = STATE_BT_HID;
            BluetoothTools::runHidSafeTest();
            break;

        case DEVIL_TWIN:
            currentState = STATE_DEVIL_TWIN;
            SystemTools::disabledLabMode("Devil Twin");
            break;

        case WARDRIVER:
            currentState = STATE_WARDRIVER;
            runWardriver();
            break;

        case RFID:
            currentState = STATE_RFID;
            if (serialCommand == "rfid_dump") {
                RfidTools::dumpToSd();
            } else if (serialCommand == "rfid_list") {
                RfidTools::listDumps();
                Display::displayInfo("RFID Dumps", "Listed on serial", "", "");
                delay(1400);
            } else {
                RfidTools::runIdentify();
            }
            break;

        case READ_BLOCKS:
            currentState = STATE_READ_BLOCKS;
            if (serialCommand == "rfid_write") {
                RfidTools::writeDumpToCard(Terminal::getArg(1));
            } else {
                RfidTools::runReadBlocks();
            }
            break;

        case FILES:
            currentState = STATE_FILES;
            if (serialCommand == "sd_delete") {
                String confirm = Terminal::getArg(2);
                confirm.toLowerCase();
                SystemTools::deleteSdFile(Terminal::getArg(1), confirm == "confirm");
            } else if (serialCommand == "sd_list") {
                SystemTools::listSdFiles();
                Display::displayInfo("SD Files", "Listed on serial", "", "");
                delay(1400);
            } else {
                SystemTools::browseSdFiles();
            }
            break;

        case READ_FILES:
            currentState = STATE_READ_FILES;
            SystemTools::readSdFile(serialCommand == "sd_read" ? Terminal::getArg(1) : "");
            break;

        case PARTY_LIGHT:
            runPartyLight();
            break;

        case LIGHTOFF:
            runLightOff();
            break;

        // === Starbeam V2 Modules ===
        case WIFI_SCAN:
            currentState = STATE_WIFI_SCAN;
            Display::displayLegalWarning();
            delay(2000);
            WiFiScanner::runScanner();
            WiFiScanner::deinit();
            break;

        case WIFI_HEATMAP:
            currentState = STATE_WIFI_HEATMAP;
            Display::displayLegalWarning();
            delay(2000);
            WiFiScanner::runHeatmap();
            WiFiScanner::deinit();
            break;

        case BLE_SCAN:
            currentState = STATE_BLE_SCAN;
            Display::displayLegalWarning();
            delay(2000);
            BLEScanner::runScanner();
            BLEScanner::deinit();
            break;

        case WEBSERVER_ON:
            if (!StarbeamWebServer::isRunning()) {
                WiFiScanner::init();
                StarbeamWebServer::start();
            } else {
                Display::displayInfo("WEB SERVER", "Already running!", StarbeamWebServer::getIP(), "");
                delay(2000);
            }
            break;

        case WEBSERVER_OFF:
            if (StarbeamWebServer::isRunning()) {
                StarbeamWebServer::stop();
                WiFiScanner::deinit();
                Display::displayInfo("WEB SERVER", "Stopped", "", "");
                delay(1500);
            } else {
                Display::displayInfo("WEB SERVER", "Not running", "", "");
                delay(1500);
            }
            break;

        case WEBSERVER_STATUS:
            if (StarbeamWebServer::isRunning()) {
                Display::displayInfo("WEB SERVER", "Running", StarbeamWebServer::getIP(), "");
            } else {
                Display::displayInfo("WEB SERVER", "Stopped", "", "");
            }
            delay(2000);
            break;

        // === Security Testing ===
        case SEC_DEAUTH_TARGET: {
            int netCount = WiFiScanner::getNetworkCount();
            if (netCount == 0) {
                Display::displayInfo("Deauth Target", "No networks", "Run WiFi Scan first", "");
                delay(2000);
                break;
            }
            int target = 0;
            Display::displayInfo("Deauth Target",
                WiFiScanner::getNetwork(0)->ssid,
                "1/" + String(netCount) + " UP/DN=select",
                "SEL=start");
            while (true) {
                if (Input::isButtonPressed(BUTTON_UP)) {
                    target = (target > 0) ? target - 1 : netCount - 1;
                    Display::displayInfo("Deauth Target",
                        WiFiScanner::getNetwork(target)->ssid,
                        String(target + 1) + "/" + String(netCount) + " UP/DN=select",
                        "SEL=start");
                    nonBlockingDelay(200);
                }
                if (Input::isButtonPressed(BUTTON_DOWN)) {
                    target = (target + 1) % netCount;
                    Display::displayInfo("Deauth Target",
                        WiFiScanner::getNetwork(target)->ssid,
                        String(target + 1) + "/" + String(netCount) + " UP/DN=select",
                        "SEL=start");
                    nonBlockingDelay(200);
                }
                if (Input::isButtonPressed(BUTTON_SELECT)) { nonBlockingDelay(200); break; }
                if (Terminal::stopRequested()) { Terminal::clearStopFlag(); return; }
                yield();
            }
            currentState = STATE_SEC_DEAUTH_TARGET;
            WiFiAttack::init();
            WiFiAttack::startDeauthTargeted(target, 7);
            Display::displayInfo("Deauth Target",
                WiFiScanner::getNetwork(target)->ssid,
                "Attacking...",
                "SEL=stop");
            while (!Terminal::stopRequested() && !Input::isButtonPressed(BUTTON_SELECT)) {
                WiFiAttack::handleAttackLoop();
                nonBlockingDelay(10);
            }
            WiFiAttack::stopAttack();
            WiFiAttack::deinit();
            Terminal::clearStopFlag();
            currentState = STATE_MENU;
            break;
        }

        case SEC_DEAUTH_ALL:
            currentState = STATE_SEC_DEAUTH_ALL;
            WiFiAttack::init();
            WiFiAttack::startDeauthBroadcast(7);
            Display::displayInfo("Deauth Broadcast", "Attacking all APs", "SEL=stop", "");
            while (!Terminal::stopRequested() && !Input::isButtonPressed(BUTTON_SELECT)) {
                WiFiAttack::handleAttackLoop();
                nonBlockingDelay(10);
            }
            WiFiAttack::stopAttack();
            WiFiAttack::deinit();
            Terminal::clearStopFlag();
            currentState = STATE_MENU;
            break;

        case SEC_BEACON_FLOOD: {
            static const char* beaconSSIDs[] = {
                "FreePublicWiFi", "xfinitywifi", "ATT_WiFi",
                "CableWiFi", "Spectrum_WiFi", "_FREE_WIFI_",
                "AndroidHotspot", "iPhone"
            };
            currentState = STATE_SEC_BEACON_FLOOD;
            WiFiAttack::init();
            WiFiAttack::startBeaconFlood(beaconSSIDs, 8);
            Display::displayInfo("Beacon Flood", "Flooding 8 SSIDs", "SEL=stop", "");
            while (!Terminal::stopRequested() && !Input::isButtonPressed(BUTTON_SELECT)) {
                WiFiAttack::handleAttackLoop();
                nonBlockingDelay(10);
            }
            WiFiAttack::stopAttack();
            WiFiAttack::deinit();
            Terminal::clearStopFlag();
            currentState = STATE_MENU;
            break;
        }

        case SEC_PROBE_FLOOD: {
            const char* probeTarget = "CypherTest";
            if (WiFiScanner::getNetworkCount() > 0) {
                probeTarget = WiFiScanner::getNetwork(0)->ssid.c_str();
            }
            currentState = STATE_SEC_PROBE_FLOOD;
            WiFiAttack::init();
            WiFiAttack::startProbeFlood(probeTarget);
            Display::displayInfo("Probe Flood", String(probeTarget), "Probing...", "SEL=stop");
            while (!Terminal::stopRequested() && !Input::isButtonPressed(BUTTON_SELECT)) {
                WiFiAttack::handleAttackLoop();
                nonBlockingDelay(10);
            }
            WiFiAttack::stopAttack();
            WiFiAttack::deinit();
            Terminal::clearStopFlag();
            currentState = STATE_MENU;
            break;
        }

        case SEC_PMKID_CAPTURE: {
            int netCount = WiFiScanner::getNetworkCount();
            if (netCount == 0) {
                Display::displayInfo("PMKID Capture", "No networks", "Run WiFi Scan first", "");
                delay(2000);
                break;
            }
            int target = 0;
            Display::displayInfo("PMKID Capture",
                WiFiScanner::getNetwork(0)->ssid,
                "1/" + String(netCount) + " UP/DN=select",
                "SEL=start");
            while (true) {
                if (Input::isButtonPressed(BUTTON_UP)) {
                    target = (target > 0) ? target - 1 : netCount - 1;
                    Display::displayInfo("PMKID Capture",
                        WiFiScanner::getNetwork(target)->ssid,
                        String(target + 1) + "/" + String(netCount) + " UP/DN=select",
                        "SEL=start");
                    nonBlockingDelay(200);
                }
                if (Input::isButtonPressed(BUTTON_DOWN)) {
                    target = (target + 1) % netCount;
                    Display::displayInfo("PMKID Capture",
                        WiFiScanner::getNetwork(target)->ssid,
                        String(target + 1) + "/" + String(netCount) + " UP/DN=select",
                        "SEL=start");
                    nonBlockingDelay(200);
                }
                if (Input::isButtonPressed(BUTTON_SELECT)) { nonBlockingDelay(200); break; }
                if (Terminal::stopRequested()) { Terminal::clearStopFlag(); return; }
                yield();
            }
            currentState = STATE_SEC_PMKID_CAPTURE;
            WiFiAttack::init();
            WiFiAttack::startPMKIDCapture(target);
            Display::displayInfo("PMKID Capture",
                WiFiScanner::getNetwork(target)->ssid,
                "Listening... (2min)",
                "SEL=stop");
            while (!Terminal::stopRequested() && !Input::isButtonPressed(BUTTON_SELECT)) {
                WiFiAttack::handleAttackLoop();
                if (!WiFiAttack::isAttacking()) break;
                nonBlockingDelay(100);
            }
            WiFiAttack::stopAttack();
            WiFiAttack::deinit();
            Terminal::clearStopFlag();
            currentState = STATE_MENU;
            break;
        }

        // === Captive Portal ===
        case CAPTIVE_PORTAL: {
            if (StarbeamWebServer::isRunning()) {
                StarbeamWebServer::stop();
                WiFiScanner::deinit();
            }
            int tmplIdx = 0;
            int total = CaptivePortal::getTemplateCount();
            Display::displayInfo("Captive Portal",
                String(CaptivePortal::getTemplateName(0)),
                "1/" + String(total) + " UP/DN=select",
                "SEL=start");
            while (true) {
                if (Input::isButtonPressed(BUTTON_UP)) {
                    tmplIdx = (tmplIdx > 0) ? tmplIdx - 1 : total - 1;
                    Display::displayInfo("Captive Portal",
                        String(CaptivePortal::getTemplateName(tmplIdx)),
                        String(tmplIdx + 1) + "/" + String(total) + " UP/DN=select",
                        "SEL=start");
                    nonBlockingDelay(200);
                }
                if (Input::isButtonPressed(BUTTON_DOWN)) {
                    tmplIdx = (tmplIdx + 1) % total;
                    Display::displayInfo("Captive Portal",
                        String(CaptivePortal::getTemplateName(tmplIdx)),
                        String(tmplIdx + 1) + "/" + String(total) + " UP/DN=select",
                        "SEL=start");
                    nonBlockingDelay(200);
                }
                if (Input::isButtonPressed(BUTTON_SELECT)) { nonBlockingDelay(200); break; }
                if (Terminal::stopRequested()) { Terminal::clearStopFlag(); return; }
                yield();
            }
            currentState = STATE_CAPTIVE_PORTAL;
            CaptivePortal::init();
            CaptivePortal::start(tmplIdx);
            unsigned long lastOled = 0;
            while (!Terminal::stopRequested() && !Input::isButtonPressed(BUTTON_SELECT)) {
                CaptivePortal::handleClient();
                if (millis() - lastOled > 2000) {
                    Display::displayInfo(
                        CaptivePortal::getActiveSSID(),
                        "Clients: " + String(CaptivePortal::getConnectedClients()),
                        "Captures: " + String(CaptivePortal::getCaptureCount()),
                        "SEL=stop");
                    lastOled = millis();
                }
                yield();
            }
            CaptivePortal::stop();
            CaptivePortal::deinit();
            Terminal::clearStopFlag();
            currentState = STATE_MENU;
            break;
        }

        case CAPTIVE_PORTAL_OFF:
            if (CaptivePortal::isRunning()) {
                CaptivePortal::stop();
                CaptivePortal::deinit();
                Display::displayInfo("C-Portal", "Stopped", "", "");
            } else {
                Display::displayInfo("C-Portal", "Not running", "", "");
            }
            delay(1500);
            break;

        case CAPTIVE_PORTAL_STATUS:
            Display::displayInfo("C-Portal Status",
                CaptivePortal::isRunning() ? "RUNNING" : "STOPPED",
                "Captures: " + String(CaptivePortal::getCaptureCount()),
                "Clients: " + String(CaptivePortal::getConnectedClients()));
            delay(3000);
            break;

        // Utility
        case REC_RAW:
            if (serialCommand == "packet_record") {
                String mode = Terminal::getArg(1);
                mode.toLowerCase();
                PacketMonitor::toggleRecording(mode == "on" || mode == "1" || mode == "true");
                Display::displayInfo("Packet Record", PacketMonitor::isRecording() ? "ON" : "OFF", "", "");
                delay(1200);
            } else {
                if (PacketMonitor::isRecording()) PacketMonitor::stopRecording();
                else PacketMonitor::startRecording();
                Display::displayInfo("Packet Record", PacketMonitor::isRecording() ? "ON" : "OFF", "", "");
                delay(1200);
            }
            break;

        case PLAY_RAW:
        case SHOW_RAW:
            PacketMonitor::showRawFiles();
            break;

        case SHOW_BUFF:
            PacketMonitor::showStatus();
            break;

        case GET_RSSI:
            if (serialCommand == "channel") {
                PacketMonitor::setChannel((uint8_t)Terminal::getArg(1).toInt());
            }
            PacketMonitor::showStatus();
            break;

        case FLUSH_BUFF:
            PacketMonitor::flushBuffer();
            break;

        case STOP_ALL:
            currentState = STATE_STOP_ALL;
            SystemTools::stopAll();
            break;

        case SETTINGS:
            Display::displayInfo("SETTINGS", "Heap: " + String(ESP.getFreeHeap() / 1024) + "KB", "", "[SEL]=exit");
            while (!Input::isButtonPressed(BUTTON_SELECT)) {
                delay(100);
                yield();
            }
            delay(200);
            break;

        case HELP:
            Display::displayInfo("Help", "Cypherbox V2", "ESP32 Cyber Toolkit", "");
            Serial.println("Cypherbox V2 - Type 'help' in terminal for commands");
            delay(3000);
            break;

        default:
            Display::displayInfo("Menu", "Unsupported item", String(selectedMenuItem), "");
            Serial.printf("Menu item %d unsupported\n", selectedMenuItem);
            delay(2000);
            break;
    }
}

// ============================================================================
// Setup Function
// ============================================================================

void setup() {
    Serial.begin(115200);

    // Initialize terminal
    Terminal::init();

    Serial.println("\n========================================");
    Serial.println("  Cypherbox V2");
    Serial.println("  ESP32 Cybersecurity Toolkit");
    Serial.println("========================================");

    delay(1000);

    // Initialize input
    Input::init();

    // Initialize display
    Display::init();

    // Initialize NeoPixel (off)
    pixels.begin();
    pixels.setPixelColor(0, 0);
    pixels.show();

    // Initialize SD Card
    SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
    if (SD.begin(SD_CS)) {
        sdInitialized = true;
        Serial.println("SD Card initialized");
    } else {
        Serial.println("SD Card init failed");
    }

    // Initialize web server (don't start AP yet)
    StarbeamWebServer::init();
    RfidTools::init();

    // Display title screen
    Display::displayTitleScreen();
    delay(2000);

    // Draw initial menu
    Display::drawMenu(selectedMenuItem, firstVisibleMenuItem);

    Serial.println("Setup complete - ready!");
    Serial.printf("Free heap: %lu bytes\n", ESP.getFreeHeap());
}

// ============================================================================
// Main Loop
// ============================================================================

void loop() {
    // Process serial input (non-blocking)
    Terminal::processInput();

    // Check for serial command and execute
    if (Terminal::hasCommand()) {
        MenuItem cmd = Terminal::getCommand();

        if (currentState != STATE_MENU) {
            Serial.println("Serial command - exiting current operation");
            currentState = STATE_MENU;
            delay(100);
        }

        selectedMenuItem = cmd;
        executeSelectedMenuItem();
        Terminal::clearCommand();
        currentState = STATE_MENU;
        Display::drawMenu(selectedMenuItem, firstVisibleMenuItem);
        Input::setButtonPressedState(false);
    }

    // Check stop signal
    if (Terminal::stopRequested() && currentState != STATE_MENU) {
        Serial.println("Stop command received");
        Terminal::clearStopFlag();
        currentState = STATE_MENU;
        Display::drawMenu(selectedMenuItem, firstVisibleMenuItem);
        return;
    }

    // State machine
    switch (currentState) {
        case STATE_MENU:
            Input::handleMenuSelection(selectedMenuItem, firstVisibleMenuItem);
            Display::drawMenu(selectedMenuItem, firstVisibleMenuItem);

            if (Input::getButtonPressedState() && Input::isButtonPressed(BUTTON_SELECT)) {
                Serial.printf("Selected menu item: %d\n", selectedMenuItem);
                executeSelectedMenuItem();
                currentState = STATE_MENU;
                Display::drawMenu(selectedMenuItem, firstVisibleMenuItem);
                Input::setButtonPressedState(false);
            }
            break;

        // Web server background handling
        case STATE_WEBSERVER:
            if (StarbeamWebServer::isRunning()) {
                StarbeamWebServer::handleClient();
            }
            break;
    }

    if (StarbeamWebServer::isRunning()) {
        StarbeamWebServer::handleClient();
    }

    yield();
}
