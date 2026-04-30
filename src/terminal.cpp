// terminal.cpp - Serial Terminal/CLI Implementation for Cypherbox V2

#include "terminal.h"
#include "display.h"
#include "wifi_scanner.h"
#include "ble_scanner.h"

char Terminal::lineBuffer[BUF_LENGTH];
uint8_t Terminal::bufferIndex = 0;
bool Terminal::commandReady = false;
bool Terminal::commandFromSerial = false;
String Terminal::currentCommandLine = "";
MenuItem Terminal::pendingCommand = WIFI_SCAN;
bool Terminal::stopFlag = false;
bool Terminal::echoEnabled = true;
bool Terminal::verboseLogging = false;
bool Terminal::initialized = false;

static const CommandMapping commands[] = {
    // Cypherbox Original
    {"packet_mon", PACKET_MON, "Packet Monitor"},
    {"wifi_sniff", WIFI_SNIFF, "WiFi Sniffer"},
    {"ap_scan", AP_SCAN, "AP Scan"},
    {"ap_join", AP_JOIN, "AP Join"},
    {"ap_create", AP_CREATE, "AP Create"},
    {"stop_ap", STOP_AP, "Stop AP"},
    {"stop_server", STOP_SERVER, "Stop Server"},
    {"bt_scan", BT_SCAN, "Bluetooth Scan"},
    {"bt_create", BT_CREATE, "Bluetooth Create"},
    {"bt_serial", BT_SERIAL_CMD, "BT Serial Command"},
    {"bt_hid", BT_HID, "BT HID Safe Test"},
    {"devil_twin", DEVIL_TWIN, "Disabled lab mode"},
    {"rfid", RFID, "RFID Reader"},
    {"read_blocks", READ_BLOCKS, "Read RFID Blocks"},
    {"wardriver", WARDRIVER, "GPS Wardriver"},
    {"party_light", PARTY_LIGHT, "Party Light"},
    {"light_off", LIGHTOFF, "Lights Off"},
    {"files", FILES, "SD Card Files"},
    {"read_files", READ_FILES, "Read SD Files"},
    // Starbeam V2 Modules
    {"wifi_scan", WIFI_SCAN, "WiFi Network Scanner"},
    {"wifi_heatmap", WIFI_HEATMAP, "WiFi Channel Heatmap"},
    {"ble_scan", BLE_SCAN, "BLE Device Scanner"},
    {"web_on", WEBSERVER_ON, "Start Web Server"},
    {"web_off", WEBSERVER_OFF, "Stop Web Server"},
    {"web_status", WEBSERVER_STATUS, "Web Server Status"},
    // Recording & Utility
    {"rec_raw", REC_RAW, "Record Raw Signal"},
    {"play_raw", PLAY_RAW, "Replay Raw Signal"},
    {"show_raw", SHOW_RAW, "Show Raw Data"},
    {"show_buff", SHOW_BUFF, "Show Buffer Contents"},
    {"get_rssi", GET_RSSI, "Get RSSI Value"},
    {"flush_buff", FLUSH_BUFF, "Clear Buffer"},
    {"stop_all", STOP_ALL, "Stop All Operations"},
    {"settings", SETTINGS, "Settings"},
    {"help", HELP, "Show Help"}
};

static const int numCommands = sizeof(commands) / sizeof(commands[0]);

void Terminal::init() {
    if (initialized) return;
    bufferIndex = 0;
    lineBuffer[0] = '\0';
    commandReady = false;
    commandFromSerial = false;
    currentCommandLine = "";
    stopFlag = false;
    initialized = true;
    Serial.println("\n=== Cypherbox V2 Terminal ===");
    Serial.println("Type 'help' for commands, 'menu' for list");
}

void Terminal::processInput() {
    if (!initialized) return;
    while (Serial.available() > 0) {
        char c = Serial.read();
        if (c == '\n' || c == '\r') {
            if (bufferIndex > 0) {
                lineBuffer[bufferIndex] = '\0';
                parseCommand(String(lineBuffer));
                bufferIndex = 0;
                lineBuffer[0] = '\0';
            }
        } else if (bufferIndex < BUF_LENGTH - 1) {
            lineBuffer[bufferIndex++] = c;
        }
    }
}

void Terminal::parseCommand(const String& cmd) {
    String trimmedCmd = cmd;
    trimmedCmd.trim();
    if (trimmedCmd.length() == 0) return;
    currentCommandLine = trimmedCmd;

    if (trimmedCmd == "help") {
        printHelp();
        return;
    }
    if (trimmedCmd == "menu") {
        printMenu();
        return;
    }
    if (trimmedCmd == "status") {
        printStatus();
        return;
    }
    if (trimmedCmd == "stop") {
        stopFlag = true;
        Serial.println("STOP signal received");
        return;
    }
    if (trimmedCmd == "wifi_list") {
        WiFiScanner::listAll();
        return;
    }
    if (trimmedCmd == "ble_list") {
        BLEScanner::listAll();
        return;
    }

    String name = getCommandName();
    if (name == "wifi_join") {
        pendingCommand = AP_JOIN;
        commandReady = true;
        commandFromSerial = true;
        return;
    }
    if (name == "rfid_dump") {
        pendingCommand = RFID;
        commandReady = true;
        commandFromSerial = true;
        return;
    }
    if (name == "rfid_write") {
        pendingCommand = READ_BLOCKS;
        commandReady = true;
        commandFromSerial = true;
        return;
    }
    if (name == "rfid_list") {
        pendingCommand = RFID;
        commandReady = true;
        commandFromSerial = true;
        return;
    }
    if (name == "sd_list") {
        pendingCommand = FILES;
        commandReady = true;
        commandFromSerial = true;
        return;
    }
    if (name == "sd_read") {
        pendingCommand = READ_FILES;
        commandReady = true;
        commandFromSerial = true;
        return;
    }
    if (name == "sd_delete") {
        pendingCommand = FILES;
        commandReady = true;
        commandFromSerial = true;
        return;
    }
    if (name == "packet_record") {
        pendingCommand = REC_RAW;
        commandReady = true;
        commandFromSerial = true;
        return;
    }
    if (name == "channel") {
        pendingCommand = GET_RSSI;
        commandReady = true;
        commandFromSerial = true;
        return;
    }

    MenuItem item = commandToMenuItem(trimmedCmd);
    if (item != (MenuItem)-1) {
        pendingCommand = item;
        commandReady = true;
        commandFromSerial = true;
        Serial.printf("CMD: %s -> %d\n", trimmedCmd.c_str(), item);
    } else {
        Serial.printf("Unknown command: %s\n", trimmedCmd.c_str());
        Serial.println("Type 'help' for available commands");
    }
}

MenuItem Terminal::commandToMenuItem(const String& cmd) {
    for (int i = 0; i < numCommands; i++) {
        if (cmd.equalsIgnoreCase(String(commands[i].command))) {
            return commands[i].menuItem;
        }
    }
    return (MenuItem)-1;
}

void Terminal::printHelp() {
    Serial.println("\n=== Cypherbox V2 Commands ===");
    Serial.println("  help         - Show this help");
    Serial.println("  menu         - List all menu items");
    Serial.println("  status       - System status");
    Serial.println("  stop         - Stop current operation");
    Serial.println("  wifi_list    - List scanned WiFi networks");
    Serial.println("  ble_list     - List scanned BLE devices");
    Serial.println("  wifi_join <ssid> <password>");
    Serial.println("  rfid_dump | rfid_list | rfid_write <dump>");
    Serial.println("  sd_list | sd_read <file> | sd_delete <file> confirm");
    Serial.println("  packet_record on|off | channel <1-13>");
    Serial.println("");
    Serial.println("=== Feature Commands ===");
    for (int i = 0; i < numCommands; i++) {
        Serial.printf("  %-16s - %s\n", commands[i].command, commands[i].description);
    }
}

void Terminal::printStatus() {
    Serial.println("\n=== Cypherbox V2 Status ===");
    Serial.printf("  Heap Free:   %lu bytes\n", ESP.getFreeHeap());
    Serial.printf("  Chip Model:  %s\n", ESP.getChipModel());
    Serial.printf("  WiFi Init:   %s\n", WiFiScanner::isInitialized() ? "YES" : "NO");
    Serial.printf("  BLE Init:    %s\n", BLEScanner::isInitialized() ? "YES" : "NO");
}

void Terminal::printMenu() {
    Serial.println("\n=== Menu Items ===");
    for (int i = 0; i < numCommands; i++) {
        Serial.printf("  [%2d] %-16s - %s\n", commands[i].menuItem, commands[i].command, commands[i].description);
    }
}

bool Terminal::hasCommand() { return commandReady; }
MenuItem Terminal::getCommand() { return pendingCommand; }
void Terminal::clearCommand() {
    commandReady = false;
    commandFromSerial = false;
}
bool Terminal::stopRequested() { return stopFlag; }
void Terminal::clearStopFlag() { stopFlag = false; }

String Terminal::getCommandName() {
    String line = currentCommandLine;
    line.trim();
    int space = line.indexOf(' ');
    String name = space >= 0 ? line.substring(0, space) : line;
    name.toLowerCase();
    return name;
}

String Terminal::getArg(uint8_t index) {
    String line = currentCommandLine;
    line.trim();
    uint8_t current = 0;
    int start = 0;
    while (start < line.length()) {
        while (start < line.length() && line.charAt(start) == ' ') start++;
        int end = line.indexOf(' ', start);
        if (end < 0) end = line.length();
        if (current == index) return line.substring(start, end);
        current++;
        start = end + 1;
    }
    return "";
}

String Terminal::getArgsAfter(uint8_t index) {
    String line = currentCommandLine;
    line.trim();
    uint8_t current = 0;
    int start = 0;
    while (start < line.length()) {
        while (start < line.length() && line.charAt(start) == ' ') start++;
        if (current == index) return line.substring(start);
        int end = line.indexOf(' ', start);
        if (end < 0) return "";
        current++;
        start = end + 1;
    }
    return "";
}

void Terminal::echoToSerial(const String& line1, const String& line2,
                            const String& line3, const String& line4) {
    if (!echoEnabled) return;
    Serial.println("---");
    Serial.println(line1);
    if (line2.length() > 0) Serial.println(line2);
    if (line3.length() > 0) Serial.println(line3);
    if (line4.length() > 0) Serial.println(line4);
}
