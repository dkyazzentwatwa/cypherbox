#ifndef MENU_MANAGER_H
#define MENU_MANAGER_H

enum MainMenuItem {
  PACKET_MON,
  WIFI_SNIFF,
  AP_SCAN,
  AP_JOIN,
  AP_CREATE,
  STOP_AP,
  STOP_SERVER,
  BT_SCAN,
  BT_CREATE,
  BT_SERIAL_CMD,
  BT_HID,
  IPHONE_SPAM,
  BEACON_SWARM,
  DEVIL_TWIN,
  RFID_READ,
  RF_SCAN,
  PARTY_LIGHT,
  FILES,
  ESPEE_BOT,
  DEAUTH,
  GAMES,
  SETTINGS,
  HELP,
  NUM_MAIN_MENU_ITEMS
};

enum PacketMonitorMenuItem {
    PACKET_MON_START,
    PACKET_MON_STOP,
    PACKET_MON_LOG,
    NUM_PACKET_MON_MENU_ITEMS
};

enum WiFiSniffMenuItem {
    WIFI_SNIFF_START,
    WIFI_SNIFF_STOP,
    WIFI_SNIFF_LOG,
    NUM_WIFI_SNIFF_MENU_ITEMS
};

enum APScanMenuItem {
    AP_SCAN_START,
    AP_SCAN_STOP,
    AP_SCAN_RESULTS,
    NUM_AP_SCAN_MENU_ITEMS
};

enum APJoinMenuItem {
    AP_JOIN_SELECT,
    AP_JOIN_PASSWORD,
    AP_JOIN_CONNECT,
    AP_JOIN_DISCONNECT,
    NUM_AP_JOIN_MENU_ITEMS
};

enum APCreateMenuItem {
    AP_CREATE_SSID,
    AP_CREATE_PASSWORD,
    AP_CREATE_START,
    AP_CREATE_STOP,
    NUM_AP_CREATE_MENU_ITEMS
};

enum BTScanMenuItem {
    BT_SCAN_START,
    BT_SCAN_STOP,
    BT_SCAN_RESULTS,
    NUM_BT_SCAN_MENU_ITEMS
};

enum BTCreateMenuItem {
    BT_CREATE_NAME,
    BT_CREATE_START,
    BT_CREATE_STOP,
    NUM_BT_CREATE_MENU_ITEMS
};

enum BTSerialCMDMenuItem {
    BT_SERIAL_CONNECT,
    BT_SERIAL_SEND,
    BT_SERIAL_DISCONNECT,
    NUM_BT_SERIAL_CMD_MENU_ITEMS
};

enum BTHIDMenuItem {
    BT_HID_CONNECT,
    BT_HID_SEND,
    BT_HID_DISCONNECT,
    NUM_BT_HID_MENU_ITEMS
};

enum iPhoneSpamMenuItem {
    IPHONE_SPAM_START,
    IPHONE_SPAM_STOP,
    IPHONE_SPAM_SET_MSG,
    NUM_IPHONE_SPAM_MENU_ITEMS
};

enum BeaconSwarmMenuItem {
    BEACON_SWARM_START,
    BEACON_SWARM_STOP,
    BEACON_SWARM_SET_COUNT,
    NUM_BEACON_SWARM_MENU_ITEMS
};

enum DevilTwinMenuItem {
    DEVIL_TWIN_SELECT,
    DEVIL_TWIN_CREATE,
    DEVIL_TWIN_STOP,
    NUM_DEVIL_TWIN_MENU_ITEMS
};

enum RFIDReadMenuItem {
    RFID_READ_START,
    RFID_READ_STOP,
    RFID_READ_VIEW,
    NUM_RFID_READ_MENU_ITEMS
};

enum RFScanMenuItem {
    RF_SCAN_START,
    RF_SCAN_STOP,
    RF_SCAN_VIEW,
    NUM_RF_SCAN_MENU_ITEMS
};

enum PartyLightMenuItem {
    PARTY_LIGHT_START,
    PARTY_LIGHT_STOP,
    PARTY_LIGHT_SET_PATTERN,
    NUM_PARTY_LIGHT_MENU_ITEMS
};

enum FilesMenuItem {
    FILES_LIST,
    FILES_OPEN,
    FILES_DELETE,
    NUM_FILES_MENU_ITEMS
};

enum EspeeBotMenuItem {
    ESPEE_BOT_START,
    ESPEE_BOT_STOP,
    ESPEE_BOT_SET_BEHAVIOR,
    NUM_ESPEE_BOT_MENU_ITEMS
};

enum DeauthMenuItem {
    DEAUTH_SELECT,
    DEAUTH_START,
    DEAUTH_STOP,
    NUM_DEAUTH_MENU_ITEMS
};

enum GamesMenuItem {
    GAMES_SNAKE,
    GAMES_TETRIS,
    GAMES_PONG,
    NUM_GAMES_MENU_ITEMS
};

enum SettingsMenuItem {
    SETTINGS_DISPLAY,
    SETTINGS_WIFI,
    SETTINGS_BT,
    SETTINGS_SYSTEM,
    NUM_SETTINGS_MENU_ITEMS
};

enum HelpMenuItem {
    HELP_GUIDE,
    HELP_ABOUT,
    HELP_SUPPORT,
    NUM_HELP_MENU_ITEMS
};

// Function declarations
void updateMainMenu();
void updatePacketMonitorMenu();
void updateWiFiSniffMenu();
void updateAPScanMenu();
void updateAPJoinMenu();
void updateAPCreateMenu();
void updateBTScanMenu();
void updateBTCreateMenu();
void updateBTSerCMDMenu();
void updateBTHIDMenu();
void updateiPhoneSpamMenu();
void updateBeaconSwarmMenu();
void updateDevilTwinMenu();
void updateRFIDReadMenu();
void updateRFScanMenu();
void updatePartyLightMenu();
void updateFilesMenu();
void updateEspeeBotMenu();
void updateDeauthMenu();
void updateGamesMenu();
void updateSettingsMenu();
void updateHelpMenu();

void selectMenuItem(MainMenuItem item);

#endif // MENU_MANAGER_H