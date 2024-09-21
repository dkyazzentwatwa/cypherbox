#include "MenuManager.h"
#include "DisplayManager.h"

// Example menu labels for main menu
const char* mainMenuLabels[NUM_MAIN_MENU_ITEMS] = { "Packet Monitor", "Wifi Sniff", "AP Scan", "AP Join", "AP Create", "Stop AP", "Stop Server", "BT Scan", "BT Create", "BT Ser. CMD", "BT HID", "iPhone Spam", "Beacon Swarm", "Devil Twin", "RFID Read", "RF Scan", "Party Light", "Files", "Espee Bot", "Deauth", "Games", "Settings", "Help" };

void updateMainMenu() {
    drawMenu(mainMenuLabels, NUM_MAIN_MENU_ITEMS, selectedMainMenuItem, firstVisibleMenuItem);
}

void updatePacketMonitorMenu() {
    const char* packetMonitorMenuLabels[NUM_PACKET_MON_MENU_ITEMS] = { "Start Monitoring", "Stop Monitoring", "Packet Log" };
    drawMenu(packetMonitorMenuLabels, NUM_PACKET_MON_MENU_ITEMS, selectedPacketMonitorMenuItem, firstVisibleMenuItem);
}

void updateWiFiSniffMenu() {
    const char* wifiSniffMenuLabels[NUM_WIFI_SNIFF_MENU_ITEMS] = { "Start Sniffing", "Stop Sniffing", "Sniffer Log" };
    drawMenu(wifiSniffMenuLabels, NUM_WIFI_SNIFF_MENU_ITEMS, selectedWiFiSniffMenuItem, firstVisibleMenuItem);
}

void updateAPScanMenu() {
    const char* apScanMenuLabels[NUM_AP_SCAN_MENU_ITEMS] = { "Start Scan", "Stop Scan", "Scan Results" };
    drawMenu(apScanMenuLabels, NUM_AP_SCAN_MENU_ITEMS, selectedAPScanMenuItem, firstVisibleMenuItem);
}

void updateAPJoinMenu() {
    const char* apJoinMenuLabels[NUM_AP_JOIN_MENU_ITEMS] = { "Select AP", "Enter Password", "Connect", "Disconnect" };
    drawMenu(apJoinMenuLabels, NUM_AP_JOIN_MENU_ITEMS, selectedAPJoinMenuItem, firstVisibleMenuItem);
}

void updateAPCreateMenu() {
    const char* apCreateMenuLabels[NUM_AP_CREATE_MENU_ITEMS] = { "Set SSID", "Set Password", "Create AP", "Stop AP" };
    drawMenu(apCreateMenuLabels, NUM_AP_CREATE_MENU_ITEMS, selectedAPCreateMenuItem, firstVisibleMenuItem);
}

void updateBTScanMenu() {
    const char* btScanMenuLabels[NUM_BT_SCAN_MENU_ITEMS] = { "Start BT Scan", "Stop BT Scan", "Scan Results" };
    drawMenu(btScanMenuLabels, NUM_BT_SCAN_MENU_ITEMS, selectedBTScanMenuItem, firstVisibleMenuItem);
}

void updateBTCreateMenu() {
    const char* btCreateMenuLabels[NUM_BT_CREATE_MENU_ITEMS] = { "Set BT Name", "Create BT Device", "Stop BT Device" };
    drawMenu(btCreateMenuLabels, NUM_BT_CREATE_MENU_ITEMS, selectedBTCreateMenuItem, firstVisibleMenuItem);
}

void updateBTSerCMDMenu() {
    const char* btSerCMDMenuLabels[NUM_BT_SER_CMD_MENU_ITEMS] = { "Connect Device", "Send Command", "Disconnect" };
    drawMenu(btSerCMDMenuLabels, NUM_BT_SER_CMD_MENU_ITEMS, selectedBTSerCMDMenuItem, firstVisibleMenuItem);
}

void updateBTHIDMenu() {
    const char* btHIDMenuLabels[NUM_BT_HID_MENU_ITEMS] = { "Connect as HID", "Send HID Command", "Disconnect HID" };
    drawMenu(btHIDMenuLabels, NUM_BT_HID_MENU_ITEMS, selectedBTHIDMenuItem, firstVisibleMenuItem);
}

void updateiPhoneSpamMenu() {
    const char* iPhoneSpamMenuLabels[NUM_IPHONE_SPAM_MENU_ITEMS] = { "Start iPhone Spam", "Stop iPhone Spam", "Set Spam Message" };
    drawMenu(iPhoneSpamMenuLabels, NUM_IPHONE_SPAM_MENU_ITEMS, selectediPhoneSpamMenuItem, firstVisibleMenuItem);
}

void updateBeaconSwarmMenu() {
    const char* beaconSwarmMenuLabels[NUM_BEACON_SWARM_MENU_ITEMS] = { "Start Beacon Swarm", "Stop Beacon Swarm", "Set Beacon Count" };
    drawMenu(beaconSwarmMenuLabels, NUM_BEACON_SWARM_MENU_ITEMS, selectedBeaconSwarmMenuItem, firstVisibleMenuItem);
}

void updateDevilTwinMenu() {
    const char* devilTwinMenuLabels[NUM_DEVIL_TWIN_MENU_ITEMS] = { "Select Target AP", "Create Evil Twin", "Stop Evil Twin" };
    drawMenu(devilTwinMenuLabels, NUM_DEVIL_TWIN_MENU_ITEMS, selectedDevilTwinMenuItem, firstVisibleMenuItem);
}

void updateRFIDReadMenu() {
    const char* rfidReadMenuLabels[NUM_RFID_READ_MENU_ITEMS] = { "Start RFID Read", "Stop RFID Read", "View RFID Data" };
    drawMenu(rfidReadMenuLabels, NUM_RFID_READ_MENU_ITEMS, selectedRFIDReadMenuItem, firstVisibleMenuItem);
}

void updateRFScanMenu() {
    const char* rfScanMenuLabels[NUM_RF_SCAN_MENU_ITEMS] = { "Start RF Scan", "Stop RF Scan", "View RF Signals" };
    drawMenu(rfScanMenuLabels, NUM_RF_SCAN_MENU_ITEMS, selectedRFScanMenuItem, firstVisibleMenuItem);
}

void updatePartyLightMenu() {
    const char* partyLightMenuLabels[NUM_PARTY_LIGHT_MENU_ITEMS] = { "Start Party Light", "Stop Party Light", "Set Light Pattern" };
    drawMenu(partyLightMenuLabels, NUM_PARTY_LIGHT_MENU_ITEMS, selectedPartyLightMenuItem, firstVisibleMenuItem);
}

void updateFilesMenu() {
    const char* filesMenuLabels[NUM_FILES_MENU_ITEMS] = { "List Files", "Open File", "Delete File" };
    drawMenu(filesMenuLabels, NUM_FILES_MENU_ITEMS, selectedFilesMenuItem, firstVisibleMenuItem);
}

void updateEspeeBotMenu() {
    const char* espeeBotMenuLabels[NUM_ESPEE_BOT_MENU_ITEMS] = { "Start Espee Bot", "Stop Espee Bot", "Set Bot Behavior" };
    drawMenu(espeeBotMenuLabels, NUM_ESPEE_BOT_MENU_ITEMS, selectedEspeeBotMenuItem, firstVisibleMenuItem);
}

void updateDeauthMenu() {
    const char* deauthMenuLabels[NUM_DEAUTH_MENU_ITEMS] = { "Select Target", "Start Deauth", "Stop Deauth" };
    drawMenu(deauthMenuLabels, NUM_DEAUTH_MENU_ITEMS, selectedDeauthMenuItem, firstVisibleMenuItem);
}

void updateGamesMenu() {
    const char* gamesMenuLabels[NUM_GAMES_MENU_ITEMS] = { "Snake", "Tetris", "Pong" };
    drawMenu(gamesMenuLabels, NUM_GAMES_MENU_ITEMS, selectedGamesMenuItem, firstVisibleMenuItem);
}

void updateSettingsMenu() {
    const char* settingsMenuLabels[NUM_SETTINGS_MENU_ITEMS] = { "Display Settings", "WiFi Settings", "Bluetooth Settings", "System Info" };
    drawMenu(settingsMenuLabels, NUM_SETTINGS_MENU_ITEMS, selectedSettingsMenuItem, firstVisibleMenuItem);
}

void updateHelpMenu() {
    const char* helpMenuLabels[NUM_HELP_MENU_ITEMS] = { "User Guide", "About", "Contact Support" };
    drawMenu(helpMenuLabels, NUM_HELP_MENU_ITEMS, selectedHelpMenuItem, firstVisibleMenuItem);
}