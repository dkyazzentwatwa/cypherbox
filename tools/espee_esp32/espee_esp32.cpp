#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_NeoPixel.h>
#include <WiFi.h>  // Use the ESP32 WiFi library
#include <U8g2_for_Adafruit_GFX.h>

// Screen configuration
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1  // Reset pin not used
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// NeoPixel setup
#define NEOPIXEL_PIN 27
#define NUMPIXELS 1
Adafruit_NeoPixel pixels(NUMPIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

// Buttons setup
#define UP_BUTTON_PIN 12
#define DOWN_BUTTON_PIN 13
#define SELECT_BUTTON_PIN 14
#define HOME_BUTTON_PIN 26

// U8g2 for Adafruit GFX
U8G2_FOR_ADAFRUIT_GFX u8g2_for_adafruit_gfx;

// Tamagotchi variables
int hunger = 50;
int health = 100;  // Start with full health
int level = 0;
int energy = 0;

// Wi-Fi network details
String recentNetwork = "";
String shortNetwork = "";
int recentChannel = 0;
int recentRSSI = 0;
int numTotNetwork = 0;

// Define emoji arrays for hunger, health, and network conditions
const char* hungerLow[] = { "(v_v)", "(v.v)", "(v-v)" };
const char* hungerMedium[] = { "(t_t)", "(t.t)", "(t-t)" };
const char* hungerHigh[] = { "(^.^)", "(^_^)", "(^o^)" };

const char* networkStrong[] = { "($_$)", "($_$)", "($_$)" };
const char* networkMedium[] = { "(x_X)", "(x.x)", "(x-x)" };
const char* networkWeak[] = { "(X_x)", "(X.x)", "(X-x)" };

int emojiIndex = 0;  // Single index for all arrays
// ASCII Art Emojis
String hungerEmoji = "(v_v)";
String healthEmoji = "(._.)";
String networkEmoji = "(X_x)";

String quotes[] = {
  "peace comes from within",
  "let go of attachments",
  "be kind to all beings",
  "practice compassion",
  "embrace the present moment",
  "mindfulness is key",
  "find joy in simplicity",
  "cultivate inner peace",
  "be mindful of thoughts",
  "live with intention",
  "suffering is impermanent",
  "let compassion guide you",
  "practice loving kindness",
  "find stillness within",
  "awaken your true self",
  "be grateful for life",
  "live in harmony",
  "accept imperfection",
  "find beauty in everything",
  "seek inner wisdom",
  "let go of judgment",
  "nurture your spirit",
  "practice self-awareness",
  "live with gratitude",
  "find peace in solitude",
  "let love guide your actions",
  "embrace impermanence",
  "cultivate inner strength",
  "connect with your soul",
  "live with compassion",
};

// Game state
bool gameOver = false;

// Time variables
unsigned long lastUpdate = 0;
const unsigned long updateInterval = 3000;  // Update stats every 3 seconds
unsigned long lastScanTime = 0;
const unsigned long scanInterval = 10000;  // Perform Wi-Fi scan every 4 seconds
unsigned long lastEmojiUpdate = 0;
const unsigned long emojiUpdateInterval = 4000;  // Update emoji every 2 seconds

// Flag for display update
bool displayNeedsUpdate = true;

int currentNetworkIndex = 0;
int numRecentNetworks = 0;  // Declare the variable outside of the function

int scrollOffset = 0;           // Scroll position
const int networksPerPage = 2;  // Number of networks displayed per page

void setup() {
  Serial.begin(115200);

  // Initialize display with I2C address 0x3C
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;
  }

  display.display();
  delay(2000);  // Pause for 2 seconds
  display.clearDisplay();

  // Initialize NeoPixel
  pixels.begin();
  pixels.show();  // Initialize all pixels to 'off'

  // Initialize U8g2_for_Adafruit_GFX
  u8g2_for_adafruit_gfx.begin(display);
  u8g2_for_adafruit_gfx.setFont(u8g2_font_4x6_mf);  // Set the font to a small size

  // Initialize buttons
  pinMode(UP_BUTTON_PIN, INPUT_PULLUP);
  pinMode(DOWN_BUTTON_PIN, INPUT_PULLUP);
  pinMode(SELECT_BUTTON_PIN, INPUT_PULLUP);
  pinMode(HOME_BUTTON_PIN, INPUT_PULLUP);

  // Display splash screens
  displayTitleScreen();
  delay(3000);  // Show title screen for 3 seconds
  displayInfoScreen();
  delay(5000);  // Show info screen for 5 seconds
  // Initial display
  displayTamagotchi();

  // Initialize Wi-Fi
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
}

void loop() {
  if (gameOver) return;

  // Handle button presses for scrolling
  if (digitalRead(UP_BUTTON_PIN) == LOW) {
    scrollOffset = max(0, scrollOffset - 1);
    displayNeedsUpdate = true;  // Indicate display update needed
    delay(100);                 // Debounce delay
  }

  if (digitalRead(DOWN_BUTTON_PIN) == LOW) {
    scrollOffset = min(numRecentNetworks - networksPerPage, scrollOffset + 1);
    displayNeedsUpdate = true;  // Indicate display update needed
    delay(200);                 // Debounce delay
  }

  // Update stats every updateInterval
  if (millis() - lastUpdate >= updateInterval) {
    lastUpdate = millis();
    updateStats();
  }

  // Increment network index after displaying current network
  if (displayNeedsUpdate && numRecentNetworks > 0) {
    currentNetworkIndex = (currentNetworkIndex + 1) % numRecentNetworks;  // Wrap around to the first network
  }

  // Perform Wi-Fi scan every scanInterval
  if (millis() - lastScanTime >= scanInterval) {
    lastScanTime = millis();
    scanNetworks();
    displayNeedsUpdate = true;  // Indicate display update needed
  }

  // Update emojis every emojiUpdateInterval
  if (millis() - lastEmojiUpdate >= emojiUpdateInterval) {
    lastEmojiUpdate = millis();
    updateEmojis();
  }

  // Check for game over
  if (health <= 0) {
    gameOver = true;
    displayGameOver();
    return;
  }

  // Update display if needed
  if (displayNeedsUpdate) {
    displayTamagotchi();
    displayNeedsUpdate = false;  // Reset the flag
  }

  // Fade NeoPixel
  fadeNeoPixel();
}

// For displaying network names
#define MAX_NETWORKS 20            // Define the maximum number of networks to store
String networkList[MAX_NETWORKS];  // Array to store network SSIDs

void scanNetworks() {
  int n = WiFi.scanNetworks();
  if (n > 0) {
    Serial.println("Wi-Fi networks found:");
    recentNetwork = WiFi.SSID(0);  // Get the most recent network name
    shortNetwork = recentNetwork.substring(0, 12);
    recentChannel = WiFi.channel(0);  // Get the most recent network channel
    recentRSSI = WiFi.RSSI(0);        // Get the most recent network RSSI
    numRecentNetworks = n;
    numTotNetwork += n;
    currentNetworkIndex = 0;  // Reset to the first network
    // Store network SSIDs in the array
    for (int i = 0; i < n && i < MAX_NETWORKS; ++i) {
      networkList[i] = WiFi.SSID(i) + " " + WiFi.RSSI(i) + "dBm";
      Serial.println(networkList[i]);
    }
    /*
    for (int i = 0; i < n; ++i) {
      Serial.println(WiFi.SSID(i));
    }
    */

    if (hunger > 10000) hunger = 10000;  // Cap hunger at 10000

    // Update health if there are more than 3 networks
    if (n > 1) {
      health += n * 2;
    }

    // Update level based on hunger points
    level = health / 200;

    // Update energy based on RSSI level
    if (recentRSSI > -60) {
      energy += n;
    }

    // Cap health and energy at 100
    if (health > 90000) health = 90000;
    if (energy > 90000) energy = 90000;
  }
  WiFi.scanDelete();  // Clear the scan results
}

void updateStats() {
  // Health decreases over time
  health = max(0, health - 5);
}

void updateEmojis() {
  unsigned long currentMillis = millis();
  if (currentMillis - lastUpdate >= updateInterval) {
    lastUpdate = currentMillis;

    // Update hunger emoji based on the hunger level
    if (health < 200) {
      hungerEmoji = hungerLow[emojiIndex];
    } else if (health < 1000) {
      hungerEmoji = hungerMedium[emojiIndex];
    } else {
      hungerEmoji = hungerHigh[emojiIndex];
    }

    // Update network emoji based on the RSSI level
    if (recentRSSI > -40) {
      networkEmoji = networkStrong[emojiIndex];
    } else if (recentRSSI > -70) {
      networkEmoji = networkMedium[emojiIndex];
    } else {
      networkEmoji = networkWeak[emojiIndex];
    }

    // Update index for next cycle
    emojiIndex = (emojiIndex + 1) % 3;
  }
}

String getRandomQuote() {
  int index = random(0, sizeof(quotes) / sizeof(quotes[0]));
  return quotes[index];
}

// Function to wrap text
String wrapText(const String& text, int maxWidth) {
  String wrappedText = "";
  String line = "";
  int textLength = text.length();

  for (int i = 0; i < textLength; i++) {
    String testLine = line + text[i];
    if (u8g2_for_adafruit_gfx.getUTF8Width(testLine.c_str()) <= maxWidth) {
      line = testLine;
    } else {
      wrappedText += line + "\n";
      line = text[i];
    }
  }
  wrappedText += line;
  return wrappedText;
}

void displayTamagotchi() {
  display.clearDisplay();
  int maxWidth = SCREEN_WIDTH - 1;  // Define the max width for text

  // Determine the width of the text to be displayed

  // Calculate the X coordinate to center the text
  int centerX = (SCREEN_WIDTH - 35) / 2;

  u8g2_for_adafruit_gfx.setCursor(centerX, 10);
  u8g2_for_adafruit_gfx.setFont(u8g2_font_baby_tr);  // Set the font to a small size
  //u8g2_for_adafruit_gfx.print("Tamagotchi");
  u8g2_for_adafruit_gfx.print(hungerEmoji);
  u8g2_for_adafruit_gfx.setCursor(0, 20);
  u8g2_for_adafruit_gfx.setFont(u8g2_font_baby_tr);  // Set the font to a small size
  u8g2_for_adafruit_gfx.print("H:" + String(hunger));
  //u8g2_for_adafruit_gfx.setCursor(0, 30);
  u8g2_for_adafruit_gfx.print(" HP:" + String(health));
  //u8g2_for_adafruit_gfx.setCursor(0, 40);
  u8g2_for_adafruit_gfx.print(" Lvl:" + String(level));
  //u8g2_for_adafruit_gfx.setCursor(0, 50);
  u8g2_for_adafruit_gfx.print(" En:" + String(energy));
  u8g2_for_adafruit_gfx.setCursor(0, 30);
  //u8g2_for_adafruit_gfx.println(getRandomQuote());
  // Display network details
  //u8g2_for_adafruit_gfx.setCursor(0, 40);
  if (numRecentNetworks > 0) {
    String netDetails = "new: " + shortNetwork + " Ch:" + recentChannel + " " + recentRSSI + "dBm";
    String wrappedNetDetails = wrapText(netDetails, maxWidth);
    u8g2_for_adafruit_gfx.print(wrappedNetDetails);
  } else {
    u8g2_for_adafruit_gfx.print("No networks found.");
  }
  u8g2_for_adafruit_gfx.setCursor(0, 40);
  u8g2_for_adafruit_gfx.print("found: ");
  u8g2_for_adafruit_gfx.print(numRecentNetworks);
  u8g2_for_adafruit_gfx.print(" ");
  u8g2_for_adafruit_gfx.print("total: ");
  u8g2_for_adafruit_gfx.print(numTotNetwork);
  // Display the list of networks
  u8g2_for_adafruit_gfx.setCursor(0, 50);
  for (int i = 0; i < networksPerPage && (scrollOffset + i) < numRecentNetworks; ++i) {
    u8g2_for_adafruit_gfx.println(networkList[scrollOffset + i]);
    u8g2_for_adafruit_gfx.setCursor(0, 50 + (i + 1) * 10);  // Adjust the position for each network
  }

  display.display();
}

void displayGameOver() {
  display.clearDisplay();
  u8g2_for_adafruit_gfx.setCursor(0, 20);
  u8g2_for_adafruit_gfx.print("Game Over!");
  display.display();
}

void displayTitleScreen() {
  display.clearDisplay();
  u8g2_for_adafruit_gfx.setFont(u8g2_font_adventurer_tr);  // Use a larger font for the title
  u8g2_for_adafruit_gfx.setCursor(10, 20);                 // Centered vertically
  u8g2_for_adafruit_gfx.print("ESPEE NET PET");
  //u8g2_for_adafruit_gfx.setCursor(centerX, 25); // Centered vertically
  //u8g2_for_adafruit_gfx.print("NETWORK PET");
  display.display();
}

void displayInfoScreen() {
  display.clearDisplay();
  u8g2_for_adafruit_gfx.setFont(u8g2_font_baby_tf);  // Set back to small font
  u8g2_for_adafruit_gfx.setCursor(30, 18);
  u8g2_for_adafruit_gfx.print("Welcome to ESPEE!");

  u8g2_for_adafruit_gfx.setCursor(30, 34);
  u8g2_for_adafruit_gfx.print("This is a simple");

  u8g2_for_adafruit_gfx.setCursor(30, 50);
  u8g2_for_adafruit_gfx.print("network scanning game.");

  u8g2_for_adafruit_gfx.setCursor(30, 55);
  u8g2_for_adafruit_gfx.print("Feed ESPEE");

  u8g2_for_adafruit_gfx.setCursor(30, 60``);
  u8g2_for_adafruit_gfx.print("by scanning WiFi!");

  display.display();
}

void fadeNeoPixel() {
  static uint32_t lastFadeTime = 0;
  static uint8_t fadeStep = 0;
  static uint8_t fadeDirection = 1;  // 1 for increasing, -1 for decreasing
  static bool isFadingIn = true;     // True for fading in, false for fading out
  static bool isOff = false;         // True when the pixel is off

  // Increase the delay for slower fading
  if (millis() - lastFadeTime >= (isOff ? 600 : (isFadingIn ? 100 : 300))) {  // Adjust the delay for fade speed
    lastFadeTime = millis();

    if (isOff) {
      isOff = false;
    } else {
      // Generate random color
      uint32_t color = pixels.Color(random(256), random(256), random(256));

      // Fade in/out
      if (isFadingIn) {
        if (fadeStep < 255 && fadeDirection == 1) {
          fadeStep += 5;  // Adjust the increment for fade speed
        } else {
          fadeDirection = -1;  // Change direction when reaching the end
          isFadingIn = false;  // Switch to fading out
          isOff = true;        // Turn off the pixel
        }
      } else {
        if (fadeStep > 0 && fadeDirection == -1) {
          fadeStep -= 5;  // Adjust the decrement for fade speed
        } else {
          fadeDirection = 1;  // Change direction when reaching the end
          isFadingIn = true;  // Switch to fading in
          isOff = true;       // Turn off the pixel
        }
      }

      // Apply gamma correction to the color
      color = pixels.gamma32(color);

      // Set the pixel color with fading
      pixels.setPixelColor(0, color);  // Directly use the gamma-corrected color
      pixels.show();
    }
  }
}