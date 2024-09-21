#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "esp_event_loop.h"
#include <WiFi.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_NeoPixel.h>
#include <U8g2_for_Adafruit_GFX.h>
#include <WebServer.h>
//BT
#include <BluetoothSerial.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <BleKeyboard.h>


// STATE MANAGEMENT
enum AppState {
  STATE_MENU,
  STATE_PACKET_MONITOR,
  STATE_WIFI_SNIFFER,
  STATE_AP_SCAN,
  STATE_AP_JOIN,
  STATE_AP_CREATE,
  STATE_STOP_AP,
  STATE_STOP_SERVER,
  STATE_BT_SCAN,
  STATE_BT_SERIAL,
  STATE_BT_HID,
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
  NUM_MENU_ITEMS
};

/*
enum MenuItem
{
  PACKET_MON,
  WIFI_SNIFF,
  WIFI,
  BLUETOOTH,
  DEVIL_TWIN,
  CAPTIVE_PORT,
  RFID,
  RF,
  PARTY_LIGHT,
  FILES,
  ESPEE_BOT,
  SETTINGS,
  HELP,
  NUM_MENU_ITEMS
};

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
//volatile bool stopSniffer = false; // Flag to stop the sniffer
volatile bool snifferRunning = true;

const int MAX_VISIBLE_MENU_ITEMS = 3; // Maximum number of items visible on the screen at a time
MenuItem selectedMenuItem = PACKET_MON;
int firstVisibleMenuItem = 0;


// *** PINS ***

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

#define NEOPIXEL_PIN 26
Adafruit_NeoPixel strip = Adafruit_NeoPixel(1, NEOPIXEL_PIN, NEO_RGB + NEO_KHZ800);
// LED
#define LED_PIN 19
// Buttons
#define UP_BUTTON_PIN 34
#define DOWN_BUTTON_PIN 35
#define SELECT_BUTTON_PIN 15
#define HOME_BUTTON_PIN 2

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

      const char *menuLabels[NUM_MENU_ITEMS] = {"Packet Monitor", "Wifi Sniff", "AP Scan", "AP Join", "AP Create", "Stop AP", "Stop Server", "BT Scan", "BT Create", "BT Ser. CMD", "BT HID", "iPhone Spam", "Beacon Swarm", "Devil Twin", "RFID Read", "RF Scan", "Party Light", "Files", "Espee Bot", "Deauth", "Games", "Settings", "Help"};

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
//  BT SCANNER    //
// Bluetooth Classic settings
BluetoothSerial SerialBT;
// BLE settings
BLEScan* pBLEScan;
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
  u8g2_for_adafruit_gfx.setFont(u8g2_font_adventurer_tr); // Use a larger font for the title
  u8g2_for_adafruit_gfx.setCursor(20, 40);                // Centered vertically
  u8g2_for_adafruit_gfx.print("LOADING...");
  display.display();


  delay(2000);
}
void runBTScan() {
    // Scan for BLE devices
  BLEScanResults* foundDevices = pBLEScan->start(scanTime, false);
  int bleDeviceCount = foundDevices->getCount();
  Serial.print("BLE Devices found: ");
  Serial.println(bleDeviceCount);
  // Start displaying Bluetooth devices from line 5
  int y = 5;
  // Display/print details of the most recent 5 Bluetooth devices found
  int numDevicesToDisplay = min(5, bleDeviceCount);  // Ensure we don't try to display more devices than were found
  for (int i = bleDeviceCount - numDevicesToDisplay; i < bleDeviceCount; i++) {
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
    delay(2000);  // Display each device info for 2 seconds
  }
  delay(10000);  // Wait before scanning again
}
void cleanupBTScan() {
    // Stop BLE scan
    if (pBLEScan) {
        pBLEScan->stop();
        delete pBLEScan;  // Deallocate BLEScan
        pBLEScan = nullptr;  // Avoid dangling pointer
    }

    // Optionally, you can also deinitialize BLE
    BLEDevice::deinit();
}

// END BT SCANNER //

// BT SERIAL COMMANDS //
// variables
BleKeyboard bleKeyboard("cookiekeyboard", "cypher", 100);
bool bluetoothActive = true;
String BTssid = "";
String BTpassword = "";

void initBTSerialDisplay() {
  display.clearDisplay();
  u8g2_for_adafruit_gfx.begin(display);
  u8g2_for_adafruit_gfx.setFont(u8g2_font_baby_tf);
  
  const char* commands[] = {
    "Available commands:",
    "WIFI \"ssid for spaced named\"",
    "PASS password",
    "START_WIFI",
    "STOP_WIFI",
    "SCAN_WIFI",
    "BT_SCAN",
    "STOP_BT"
  };
  
  for (int i = 0; i < 8; i++) {
    u8g2_for_adafruit_gfx.setCursor(0, i * 8);
    u8g2_for_adafruit_gfx.print(commands[i]);
  }
  
  display.display();
}

void initBTSerial() {
  // Initialize Bluetooth
  if (!SerialBT.begin("cookiemangos")) {
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
      SerialBT.println("SSID set to: " + BTssid);
      Serial.println("SSID set to: " + BTssid);
      u8g2_for_adafruit_gfx.setCursor(0, 8);
      u8g2_for_adafruit_gfx.print("SSID set: " + BTssid);
      display.display();
    } else {
      SerialBT.println("Invalid WIFI command format. Use: WIFI \"ssid for spaced named\"");
      Serial.println("Invalid WIFI command format. Use: WIFI \"ssid for spaced named\"");
    }
  } else if (command.startsWith("PASS ")) {
    BTpassword = command.substring(5);
    SerialBT.println("Password set to: " + BTpassword);
    Serial.println("Password set to: " + BTpassword);
    u8g2_for_adafruit_gfx.setCursor(0, 8);
    u8g2_for_adafruit_gfx.print("Password set");
    display.display();
  } else if (command == "START_WIFI") {
    //startWiFi();
  } else if (command == "STOP_WIFI") {
    //stopWiFi();
  } else if (command == "SCAN_WIFI") {
    //scanWiFiNetworks();
  } else if (command == "BT_SCAN") {
    SerialBT.println("BT_SCAN command received, but not implemented yet.");
  } else if (command == "BT_HID") {
    SerialBT.println("BT_HID command selected....");
    Serial.println("BT_HID command selected....");
    u8g2_for_adafruit_gfx.setCursor(0, 8);
    u8g2_for_adafruit_gfx.print("BT HID Selected.");
    //blueHID();
  } else if (command == "STOP_BT") {
    //stopBluetooth();
  } else if (command == "CREATE_AP") {
    SerialBT.println("CREATE_AP command received, but not implemented yet.");
  } else {
    SerialBT.println("Unknown command: " + command);
    Serial.println("Unknown command: " + command);
  }
}
// END BT SERIAL COMMANDS //

// BT HID START //

void initBTHid() {
  bleKeyboard.begin();

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
    const char* url = "https://youtu.be/oHg5SJYRHA0?si=bfXXjVeR5UyKoh4n";

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
    display.display();
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
    u8g2_for_adafruit_gfx.setCursor(0, 20);
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
void printOnDisplay(String message) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);

      // Set the box coordinates and size
      /*int16_t x1, y1;
      uint16_t w, h;
      display.getTextBounds(message, 0, 0, &x1, &y1, &w, &h);
      display.drawRect(x1 - 2, y1 - 2, w + 4, h + 4, SSD1306_WHITE);
      */
  display.println(message);
  display.display();
      //u8g2 addition
      /*
      display.clearDisplay()'
      u8g2_for_adafruit_gfx.setFont(u8g2_font_tom_thumb_4x6_mr); // Set back to small font
      u8g2_for_adafruit_gfx.setCursor(0, 0);
      u8g2_for_adafruit_gfx.print(message);
      display.display();
      */
}
////WIFI_SNIFFER SETUP
  #define LED_GPIO_PIN  26
  #define WIFI_CHANNEL_SWITCH_INTERVAL  (5000)
  #define WIFI_CHANNEL_MAX               (13)
  uint8_t level = 0, channel = 1;

  static wifi_country_t wifi_country = {.cc="CN", .schan = 1, .nchan = 13}; //Most recent esp32 library struct

  typedef struct {
    unsigned frame_ctrl:16;
    unsigned duration_id:16;
    uint8_t addr1[6]; /* receiver address */
    uint8_t addr2[6]; /* sender address */
    uint8_t addr3[6]; /* filtering address */
    unsigned sequence_ctrl:16;
    uint8_t addr4[6]; /* optional */
  } wifi_ieee80211_mac_hdr_t;

  typedef struct {
    wifi_ieee80211_mac_hdr_t hdr;
    uint8_t payload[0]; /* network data ended with 4 bytes csum (CRC32) */
  } wifi_ieee80211_packet_t;

  //static esp_err_t event_handler(void *ctx, system_event_t *event);
  static void wifi_sniffer_init(void);
  static void wifi_sniffer_set_channel(uint8_t channel);
  static const char *wifi_sniffer_packet_type2str(wifi_promiscuous_pkt_type_t type);
  //static void wifi_sniffer_packet_handler(void *buff, wifi_promiscuous_pkt_type_t type);

  /*esp_err_t event_handler(void *ctx, system_event_t *event)
  {
    return ESP_OK;
  }
  */
  //gemini
  static void event_handler(void* arg, esp_event_base_t event_base,
                            int32_t event_id, void* event_data)
  {
    // Handle events here
  }

void wifi_sniffer_init(void)
  {
    nvs_flash_init();
    //tcpip_adapter_init();
    //ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );
    //gemini
    ESP_ERROR_CHECK( esp_event_loop_create_default() );
    ESP_ERROR_CHECK( esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL) );
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_country(&wifi_country) ); /* set country for channel range [1, 13] */
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_NULL) );
    ESP_ERROR_CHECK( esp_wifi_start() );
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_rx_cb(&wifi_sniffer_packet_handler);
  }

void wifi_sniffer_set_channel(uint8_t channel)
  {
    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
  }

const char * wifi_sniffer_packet_type2str(wifi_promiscuous_pkt_type_t type)
  {
    switch(type) {
    case WIFI_PKT_MGMT: return "MGMT";
    case WIFI_PKT_DATA: return "DATA";
    default:  
    case WIFI_PKT_MISC: return "MISC";
    }
  }

void wifi_sniffer_packet_handler(void* buff, wifi_promiscuous_pkt_type_t type) {
  if (!snifferRunning) return; // Skip packet processing if sniffer is paused
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
void initWifiSniffer() {
  //Serial.begin(115200);
  //delay(10);
  //initDisplay(); // Initialize the display
  wifi_sniffer_init();
  pinMode(LED_GPIO_PIN, OUTPUT);

}

void runWifiSniffer() {
  static bool snifferRunning = true;
 if (isButtonPressed(HOME_BUTTON_PIN)) {
    esp_wifi_set_promiscuous(false);
    currentState = STATE_MENU;
    // Clean up WiFi sniffer if necessary
    drawMenu();
    delay(500); // Debounce delay
    return;
  }
 if (isButtonPressed(SELECT_BUTTON_PIN)) {
    snifferRunning = !snifferRunning;
    display.clearDisplay();
    display.setCursor(0,0);
    display.println(snifferRunning ? "Sniffer Running" : "Sniffer Paused");
    display.display();
    delay(500); // Debounce delay
  }

  if (snifferRunning) {
    wifi_sniffer_init();
    vTaskDelay(WIFI_CHANNEL_SWITCH_INTERVAL / portTICK_PERIOD_MS);
    wifi_sniffer_set_channel(channel);
    channel = (channel % WIFI_CHANNEL_MAX) + 1;
  } else {
    vTaskDelay(100 / portTICK_PERIOD_MS); // Short delay when paused to prevent busy-waiting
  }
}
// *** GPS MODULES ***

// *** TOOLS MODULES ***

// *** SYSTEM MODULES ***
// MENU
// LEDS SETUP
  uint32_t getRandomColor()
  {
    return strip.Color(random(256), random(256), random(256));
  }
void updateNeoPixel() {
    strip.setPixelColor(0, getRandomColor());
    strip.show();
    delay(200); // Keep the light on for 1000 milliseconds (1 second)
    strip.setPixelColor(0, 0); // Turn off the pixel
    strip.show(); // Update the strip to show the change
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
void executeSelectedMenuItem() {
  switch (selectedMenuItem) {
    /*
    case PACKET_MON:
      Serial.println("PACKET button pressed");
      currentState = STATE_PACKET_MONITOR;
      break;
    */
    case WIFI_SNIFF:
      Serial.println("WIFI SNIFF button pressed");
      currentState = STATE_WIFI_SNIFFER;
      //test
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
      runBTHID();
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

void loop() {
  switch (currentState) {
    case STATE_MENU:
      handleMenuSelection();
      break;
    case STATE_PACKET_MONITOR:
      //runPacketMonitor();
      break;
    case STATE_WIFI_SNIFFER:
      if (isButtonPressed(HOME_BUTTON_PIN)) {
          currentState = STATE_MENU;
          // Clean up WiFi sniffer if necessary
          esp_wifi_set_promiscuous(false);
          drawMenu();
          delay(500); // Debounce delay
          return;
        }
      break;
    case STATE_AP_SCAN:
      if (isButtonPressed(HOME_BUTTON_PIN)) {
          currentState = STATE_MENU;
          drawMenu();
          delay(500); // Debounce delay
          return;
        }
      break;
    case STATE_AP_JOIN:
      if (isButtonPressed(HOME_BUTTON_PIN)) {
          currentState = STATE_MENU;
          drawMenu();
          delay(500); // Debounce delay
          return;
        }      break;
    case STATE_AP_CREATE:
      if (isButtonPressed(HOME_BUTTON_PIN)) {
          currentState = STATE_MENU;
          drawMenu();
          delay(500); // Debounce delay
          return;
        }      break;
    case STATE_STOP_AP:
      if (isButtonPressed(HOME_BUTTON_PIN)) {
          currentState = STATE_MENU;
          drawMenu();
          delay(500); // Debounce delay
          return;
        }      break;
    case STATE_STOP_SERVER:
      if (isButtonPressed(HOME_BUTTON_PIN)) {
          currentState = STATE_MENU;
          drawMenu();
          delay(500); // Debounce delay
          return;
        }      break;
    case STATE_BT_SCAN:
    if (isButtonPressed(HOME_BUTTON_PIN)) {
        cleanupBTScan();
        delay(3000);
        currentState = STATE_MENU;
        drawMenu();
        delay(500); // Debounce delay
        return;
      }      break;
    case STATE_BT_SERIAL:
    if (isButtonPressed(HOME_BUTTON_PIN)) {
        currentState = STATE_MENU;
        drawMenu();
        delay(500); // Debounce delay
        return;
      }      break;
    case STATE_BT_HID:
    if (isButtonPressed(HOME_BUTTON_PIN)) {
        currentState = STATE_MENU;
        drawMenu();
        delay(500); // Debounce delay
        return;
      }      break;

  }
  if (serverRunning)
  {
    server.handleClient();
  }

}
