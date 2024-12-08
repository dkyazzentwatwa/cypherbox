#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_NeoPixel.h>
#include <U8g2_for_Adafruit_GFX.h>
#include "FreeSerifBold9pt7b.h"
#include "FreeSerifBoldItalic9pt7b.h"
#include "Org_01.h"


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
  STATE_FILES,
  STATE_READ_FILES,
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
  PARTY_LIGHT,
  LIGHTOFF,
  FILES,
  READ_FILES,
  // DEAUTH,
  SETTINGS,
  HELP,
  NUM_MENU_ITEMS
};

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

//SD CARD Variables
#define SD_CS 5
#define SD_MOSI 23  // SD Card MOSI pin
#define SD_MISO 19  // SD Card MISO pin
#define SD_SCK 18   // SD Card SCK pin
bool inMenu = true;
bool inSDMenu = false;
int currentMenuItem = 0;
int currentSDMenuItem = 0;
const int totalMenuItems = 5;
int menuIndex = 0;    // Tracks the current menu option
int SDmenuIndex = 0;  // Tracks the current menu option
// Variables for file navigation
String fileList[20];       // Array to store filenames
int fileCount = 0;         // Number of files found
int currentFileIndex = 0;  // Currently selected file

//title screen images
static const unsigned char PROGMEM image_EviSmile1_bits[] = { 0x30, 0x03, 0x00, 0x60, 0x01, 0x80, 0xe0, 0x01, 0xc0, 0xf3, 0xf3, 0xc0, 0xff, 0xff, 0xc0, 0xff, 0xff, 0xc0, 0x7f, 0xff, 0x80, 0x7f, 0xff, 0x80, 0x7f, 0xff, 0x80, 0xef, 0xfd, 0xc0, 0xe7, 0xf9, 0xc0, 0xe3, 0xf1, 0xc0, 0xe1, 0xe1, 0xc0, 0xf1, 0xe3, 0xc0, 0xff, 0xff, 0xc0, 0x7f, 0xff, 0x80, 0x7b, 0xf7, 0x80, 0x3d, 0x2f, 0x00, 0x1e, 0x1e, 0x00, 0x0f, 0xfc, 0x00, 0x03, 0xf0, 0x00 };
static const unsigned char PROGMEM image_Ble_connected_bits[] = { 0x07, 0xc0, 0x1f, 0xf0, 0x3e, 0xf8, 0x7e, 0x7c, 0x76, 0xbc, 0xfa, 0xde, 0xfc, 0xbe, 0xfe, 0x7e, 0xfc, 0xbe, 0xfa, 0xde, 0x76, 0xbc, 0x7e, 0x7c, 0x3e, 0xf8, 0x1f, 0xf0, 0x07, 0xc0 };
static const unsigned char PROGMEM image_MHz_bits[] = { 0xc3, 0x61, 0x80, 0x00, 0xe7, 0x61, 0x80, 0x00, 0xff, 0x61, 0x80, 0x00, 0xff, 0x61, 0xbf, 0x80, 0xdb, 0x7f, 0xbf, 0x80, 0xdb, 0x7f, 0x83, 0x00, 0xdb, 0x61, 0x86, 0x00, 0xc3, 0x61, 0x8c, 0x00, 0xc3, 0x61, 0x98, 0x00, 0xc3, 0x61, 0xbf, 0x80, 0xc3, 0x61, 0xbf, 0x80 };
static const unsigned char PROGMEM image_Error_bits[] = { 0x03, 0xf0, 0x00, 0x0f, 0xfc, 0x00, 0x1f, 0xfe, 0x00, 0x3f, 0xff, 0x00, 0x73, 0xf3, 0x80, 0x71, 0xe3, 0x80, 0xf8, 0xc7, 0xc0, 0xfc, 0x0f, 0xc0, 0xfe, 0x1f, 0xc0, 0xfe, 0x1f, 0xc0, 0xfc, 0x0f, 0xc0, 0xf8, 0xc7, 0xc0, 0x71, 0xe3, 0x80, 0x73, 0xf3, 0x80, 0x3f, 0xff, 0x00, 0x1f, 0xfe, 0x00, 0x0f, 0xfc, 0x00, 0x03, 0xf0, 0x00 };
static const unsigned char PROGMEM image_Bluetooth_Idle_bits[] = { 0x20, 0xb0, 0x68, 0x30, 0x30, 0x68, 0xb0, 0x20 };
static const unsigned char PROGMEM image_off_text_bits[] = { 0x67, 0x70, 0x94, 0x40, 0x96, 0x60, 0x94, 0x40, 0x64, 0x40 };
static const unsigned char PROGMEM image_wifi_not_connected_bits[] = { 0x21, 0xf0, 0x00, 0x16, 0x0c, 0x00, 0x08, 0x03, 0x00, 0x25, 0xf0, 0x80, 0x42, 0x0c, 0x40, 0x89, 0x02, 0x20, 0x10, 0xa1, 0x00, 0x23, 0x58, 0x80, 0x04, 0x24, 0x00, 0x08, 0x52, 0x00, 0x01, 0xa8, 0x00, 0x02, 0x04, 0x00, 0x00, 0x42, 0x00, 0x00, 0xa1, 0x00, 0x00, 0x40, 0x80, 0x00, 0x00, 0x00 };
static const unsigned char PROGMEM image_volume_muted_bits[] = { 0x01, 0xc0, 0x00, 0x02, 0x40, 0x00, 0x04, 0x40, 0x00, 0x08, 0x40, 0x00, 0xf0, 0x50, 0x40, 0x80, 0x48, 0x80, 0x80, 0x45, 0x00, 0x80, 0x42, 0x00, 0x80, 0x45, 0x00, 0x80, 0x48, 0x80, 0xf0, 0x50, 0x40, 0x08, 0x40, 0x00, 0x04, 0x40, 0x00, 0x02, 0x40, 0x00, 0x01, 0xc0, 0x00, 0x00, 0x00, 0x00 };
static const unsigned char PROGMEM image_network_not_connected_bits[] = { 0x82, 0x0e, 0x44, 0x0a, 0x28, 0x0a, 0x10, 0x0a, 0x28, 0xea, 0x44, 0xaa, 0x82, 0xaa, 0x00, 0xaa, 0x0e, 0xaa, 0x0a, 0xaa, 0x0a, 0xaa, 0x0a, 0xaa, 0xea, 0xaa, 0xaa, 0xaa, 0xee, 0xee, 0x00, 0x00 };
static const unsigned char PROGMEM image_microphone_muted_bits[] = { 0x87, 0x00, 0x4f, 0x80, 0x26, 0x80, 0x13, 0x80, 0x09, 0x80, 0x04, 0x80, 0x0a, 0x00, 0x0d, 0x00, 0x2e, 0xa0, 0x27, 0x40, 0x10, 0x20, 0x0f, 0x90, 0x02, 0x08, 0x02, 0x04, 0x0f, 0x82, 0x00, 0x00 };
static const unsigned char PROGMEM image_cross_contour_bits[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x80, 0x51, 0x40, 0x8a, 0x20, 0x44, 0x40, 0x20, 0x80, 0x11, 0x00, 0x20, 0x80, 0x44, 0x40, 0x8a, 0x20, 0x51, 0x40, 0x20, 0x80, 0x00, 0x00, 0x00, 0x00 };
static const unsigned char PROGMEM image_mute_text_bits[] = { 0x8a, 0x5d, 0xe0, 0xda, 0x49, 0x00, 0xaa, 0x49, 0xc0, 0x8a, 0x49, 0x00, 0x89, 0x89, 0xe0 };
//loading images
static const unsigned char PROGMEM image_LoadingHourglass_bits[] = { 0x00, 0x00, 0x00, 0x07, 0xff, 0xf0, 0x04, 0x00, 0x10, 0x03, 0xff, 0xe0, 0x01, 0x00, 0x40, 0x01, 0x55, 0x40, 0x01, 0x2a, 0x40, 0x01, 0x14, 0x40, 0x00, 0x88, 0x80, 0x00, 0x41, 0x00, 0x00, 0x2a, 0x00, 0x00, 0x14, 0x00, 0x00, 0x14, 0x00, 0x00, 0x22, 0x00, 0x00, 0x49, 0x00, 0x00, 0x80, 0x80, 0x01, 0x00, 0x40, 0x01, 0x00, 0x40, 0x01, 0x00, 0x40, 0x01, 0x00, 0x40, 0x03, 0xff, 0xe0, 0x04, 0x00, 0x10, 0x07, 0xff, 0xf0, 0x00, 0x00, 0x00 };
static const unsigned char PROGMEM image_hourglass4_bits[] = { 0x00, 0x40, 0x00, 0x00, 0xe0, 0x00, 0x01, 0x40, 0x00, 0x02, 0xa0, 0x00, 0x05, 0x10, 0x00, 0x0a, 0x08, 0x00, 0x14, 0x08, 0x00, 0x28, 0x08, 0x00, 0x50, 0x08, 0x00, 0xe0, 0x08, 0x00, 0x50, 0x08, 0x00, 0x08, 0x07, 0xe0, 0x07, 0xe0, 0x10, 0x00, 0x14, 0x0a, 0x00, 0x17, 0xe7, 0x00, 0x17, 0xca, 0x00, 0x17, 0x94, 0x00, 0x17, 0x28, 0x00, 0x12, 0x50, 0x00, 0x08, 0xa0, 0x00, 0x05, 0x40, 0x00, 0x02, 0x80, 0x00, 0x07, 0x00, 0x00, 0x02, 0x00 };
static const unsigned char PROGMEM image_hourglass5_bits[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x00, 0x06, 0x50, 0x00, 0x0a, 0x5f, 0x00, 0xfa, 0x50, 0x81, 0x0a, 0x50, 0x42, 0x0a, 0x50, 0x24, 0x0a, 0x50, 0x18, 0x7a, 0x50, 0x03, 0xfa, 0x50, 0x19, 0xfa, 0x50, 0x24, 0xfa, 0x50, 0x42, 0x7a, 0x50, 0x81, 0x0a, 0x5f, 0x00, 0xfa, 0x50, 0x00, 0x0a, 0x60, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
static const unsigned char PROGMEM image_hourglass6_bits[] = { 0x00, 0x02, 0x00, 0x00, 0x07, 0x00, 0x00, 0x02, 0x80, 0x00, 0x05, 0x40, 0x00, 0x08, 0xa0, 0x00, 0x10, 0x50, 0x00, 0x10, 0x28, 0x00, 0x13, 0x94, 0x00, 0x17, 0xca, 0x00, 0x17, 0xe7, 0x00, 0x17, 0xca, 0x07, 0xe0, 0x10, 0x08, 0x07, 0xe0, 0x50, 0x08, 0x00, 0xe0, 0x08, 0x00, 0x50, 0x08, 0x00, 0x28, 0x08, 0x00, 0x14, 0x08, 0x00, 0x0a, 0x08, 0x00, 0x05, 0x10, 0x00, 0x02, 0xa0, 0x00, 0x01, 0x40, 0x00, 0x00, 0xe0, 0x00, 0x00, 0x40, 0x00 };
static const unsigned char PROGMEM image_bluetooth_not_connected_bits[] = { 0x02, 0x00, 0x83, 0x00, 0x42, 0x80, 0x22, 0x40, 0x12, 0x20, 0x08, 0x20, 0x04, 0x40, 0x02, 0x00, 0x05, 0x00, 0x0a, 0x80, 0x12, 0x40, 0x22, 0x20, 0x02, 0x50, 0x02, 0x88, 0x01, 0x04, 0x00, 0x00 };
// *** DISPLAY
void initDisplay() {
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {  // Address 0x3C for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;
  }
  display.clearDisplay();
}
void drawBorder() {
  display.drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_WHITE);
}
void drawMenu() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.fillRect(0, 0, SCREEN_WIDTH, YELLOW_HEIGHT, SSD1306_WHITE);  // Fill the top 16px with white (yellow on your screen)
  display.setTextColor(SSD1306_BLACK);                                 // Black text in yellow area
  display.setCursor(5, 4);                                             // Adjust as needed
  display.setTextSize(1);
  display.println("Home");  // Replace with dynamic title if needed

  // Edit this to edit the menu items. Linked with AppState and MenuItem.
  const char *menuLabels[NUM_MENU_ITEMS] = { "Packet Monitor", "Wifi Sniff", "AP Scan", "AP Join", "AP Create", "Stop AP", "Stop Server", "BT Scan", "BT Create", "BT Ser. CMD", "BT HID", "Devil Twin", "RFID Read", "RFID Read Blocks", "Wardriver",
                                             "Party Light", "Light Off", "View Files", "Read Files", "Settings", "Help" };
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

void displayTitleScreen2() {
  display.clearDisplay();
  u8g2_for_adafruit_gfx.setFont(u8g2_font_adventurer_tr);  // Use a larger font for the title
  u8g2_for_adafruit_gfx.setCursor(20, 40);                 // Centered vertically
  display.drawBitmap(56, 40, image_EviSmile1_bits, 18, 21, 1);
  display.setTextWrap(false);
  u8g2_for_adafruit_gfx.setCursor(30, 18);
  u8g2_for_adafruit_gfx.print("C y p h e r");
  u8g2_for_adafruit_gfx.setCursor(40, 35);
  u8g2_for_adafruit_gfx.print("B O X");
  display.drawBitmap(106, 19, image_Ble_connected_bits, 15, 15, 1);
  display.drawBitmap(2, 50, image_MHz_bits, 25, 11, 1);
  display.drawBitmap(1, 1, image_Error_bits, 18, 18, 1);
  display.drawBitmap(25, 38, image_Bluetooth_Idle_bits, 5, 8, 1);
  display.drawBitmap(83, 55, image_off_text_bits, 12, 5, 1);
  display.drawBitmap(109, 2, image_wifi_not_connected_bits, 19, 16, 1);
  display.drawBitmap(4, 31, image_volume_muted_bits, 18, 16, 1);
  display.drawBitmap(109, 45, image_network_not_connected_bits, 15, 16, 1);
  display.drawBitmap(92, 33, image_microphone_muted_bits, 15, 16, 1);
  display.drawBitmap(1, 23, image_mute_text_bits, 19, 5, 1);
  display.drawBitmap(32, 49, image_cross_contour_bits, 11, 16, 1);
  display.display();
}
void displayTitleScreen3() {
  display.clearDisplay();
  display.setTextWrap(false);
  u8g2_for_adafruit_gfx.setFont(u8g2_font_6x10_mr);
  u8g2_for_adafruit_gfx.setCursor(6, 7);
  u8g2_for_adafruit_gfx.print("little hakr presents");
  u8g2_for_adafruit_gfx.setFont(u8g2_font_4x6_tn);
  u8g2_for_adafruit_gfx.setCursor(0, 14);
  u8g2_for_adafruit_gfx.print("------------------------------------------------------");
  u8g2_for_adafruit_gfx.setFont(u8g2_font_6x12_me);
  u8g2_for_adafruit_gfx.setCursor(29, 50);
  u8g2_for_adafruit_gfx.print("C y p h e r");
  u8g2_for_adafruit_gfx.setCursor(48, 61);
  u8g2_for_adafruit_gfx.print("B O X");
  display.drawBitmap(109, 22, image_Ble_connected_bits, 15, 15, 1);
  display.drawBitmap(2, 50, image_MHz_bits, 25, 11, 1);
  display.drawBitmap(92, 23, image_off_text_bits, 12, 5, 1);
  display.drawBitmap(4, 31, image_volume_muted_bits, 18, 16, 1);
  display.drawBitmap(22, 21, image_bluetooth_not_connected_bits, 14, 16, 1);
  display.drawBitmap(109, 45, image_network_not_connected_bits, 15, 16, 1);
  display.drawBitmap(96, 33, image_microphone_muted_bits, 15, 16, 1);
  display.drawBitmap(67, 21, image_cross_contour_bits, 11, 16, 1);
  display.drawBitmap(42, 29, image_mute_text_bits, 19, 5, 1);
  display.display();
}
void loadingScreen() {
  display.clearDisplay();
  u8g2_for_adafruit_gfx.setFont(u8g2_font_adventurer_tr);  // Use a larger font for the title
  display.drawBitmap(0, 38, image_LoadingHourglass_bits, 24, 24, 1);
  display.setTextWrap(false);
  u8g2_for_adafruit_gfx.setCursor(16, 29);
  u8g2_for_adafruit_gfx.print("l o a d i n g . .");
  display.drawBitmap(30, 37, image_hourglass4_bits, 24, 24, 1);
  display.drawBitmap(63, 37, image_hourglass5_bits, 24, 24, 1);
  display.drawBitmap(98, 37, image_hourglass6_bits, 24, 24, 1);
  display.display();
}

void itemLoadingScreen(const String &Text) {
  display.clearDisplay();

  u8g2_for_adafruit_gfx.setFont(u8g2_font_adventurer_tr);  // Use a larger font for the title
  display.drawBitmap(0, 38, image_LoadingHourglass_bits, 24, 24, 1);

  display.setTextWrap(false);
  u8g2_for_adafruit_gfx.setCursor(15, 31);
  u8g2_for_adafruit_gfx.print("l o a d i n g . .");

  u8g2_for_adafruit_gfx.setFont(u8g2_font_baby_tf);  // Set back to small font

  // Set cursor for Bluetooth text without changing the position
  u8g2_for_adafruit_gfx.setCursor(0, 5);
  u8g2_for_adafruit_gfx.print(Text);  // Print the passed Bluetooth text

  // Draw hourglass images
  display.drawBitmap(30, 37, image_hourglass4_bits, 24, 24, 1);
  display.drawBitmap(63, 37, image_hourglass5_bits, 24, 24, 1);
  display.drawBitmap(98, 37, image_hourglass6_bits, 24, 24, 1);

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
  // Display splash screens
  displayTitleScreen3();
  //delay(3000);
  //itemLoadingScreen("->>> wifi ap");
  delay(5000);

  //delay(2000);  // Show title screen for 3 seconds
  //displayInfoScreen();
  u8g2_for_adafruit_gfx.setFont(u8g2_font_baby_tf);  // Set back to small font

  drawMenu();
}

void loop() {
}
