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

// *** BLUETOOTH MODULES ***

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
const char *ssid = "your_wifi";
const char *password = "12345";
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

// Create a web server object
WebServer server(80);
bool serverRunning = false;
// Function to handle the root path (/)
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

// *** GPS MODULES ***

// *** TOOLS MODULES ***

// *** SYSTEM MODULES ***
// MENU
const int MAX_VISIBLE_MENU_ITEMS = 3; // Maximum number of items visible on the screen at a time
MenuItem selectedMenuItem = PACKET_MON;
int firstVisibleMenuItem = 0;
// LEDS
uint32_t getRandomColor()
{
  return strip.Color(random(256), random(256), random(256));
}
void updateNeoPixel()
{
  strip.setPixelColor(0, getRandomColor());
  strip.show();
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
    // Execute code for Packet Monitor
    Serial.println("PACKET button pressed");
    break;
  case WIFI_SNIFF:
    // Execute code for AP Scan
    Serial.println("WIFI SNIFF button pressed");
    break;
  case AP_SCAN:
    // Execute code for AP Scan
    Serial.println("AP SCAN button pressed");
    performScan();
    delay(2000);          // Scan for networks
    displayNetworkScan(); // Display the scanned networks
    break;
  case AP_JOIN:
    // Execute code for AP Create
    Serial.println("AP Join button pressed");
    startServer();
    delay(2000); // Scan for networks
    break;
  case AP_CREATE:
    // Execute code for AP Create
    Serial.println("AP Create button pressed");
    startSoftAP();
    delay(2000); // Scan for networks
    break;
  case STOP_AP:
    // Execute code for AP Create
    Serial.println("STOP AP button pressed");
    handleStopServer();
    delay(2000); // Scan for networks
    break;
  case STOP_SERVER:
    // Execute code for AP Create
    Serial.println("STOP SERVER button pressed");
    handleStopServer();
    delay(2000); // Scan for networks
    break;
  case IPHONE_SPAM:
    // Execute code for AP Scan
    Serial.println("IPHONE SPAM button pressed");
    break;
  case BEACON_SWARM:
    // Execute code for AP Scan
    Serial.println("BEACON SWARM button pressed");
    break;
  case DEVIL_TWIN:
    // Execute code for AP Scan
    Serial.println("DEVIL_TWIN button pressed");
    break;
  case FILES:
    // Execute code for Files
    Serial.println("FILES button pressed");
    break;
  case ESPEE_BOT:
    // Execute code for Espee Bot
    Serial.println("ESPEE button pressed");
    break;
  case DEAUTH:
    // Execute code for Deauth
    Serial.println("DEAUTH button pressed");
    break;
  case GAMES:
    // Execute code for Deauth
    Serial.println("GAMES button pressed");
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
  strip.begin();
  strip.setPixelColor(0, strip.Color(255, 0, 0)); // Bright red
  strip.show();                                   // Initialize all pixels to 'off'

  // Display splash screens
  displayTitleScreen();
  delay(3000); // Show title screen for 3 seconds
  displayInfoScreen();
  delay(5000); // Show info screen for 5 seconds
  // Initial display
  drawMenu();
}

void loop()
{
  handleMenuSelection();
  if (serverRunning)
  {
    server.handleClient();
  }
  delay(100); // Adjust delay as needed
}