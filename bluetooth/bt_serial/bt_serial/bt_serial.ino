#include <BluetoothSerial.h>
#include <WiFi.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <U8g2_for_Adafruit_GFX.h>
#include <Wire.h>
#include <BleKeyboard.h>

U8G2_FOR_ADAFRUIT_GFX u8g2_for_adafruit_gfx;
BluetoothSerial SerialBT;
BleKeyboard bleKeyboard("cookiekeyboard", "cypher", 100);

#define SCREEN_WIDTH 128  // OLED display width, in pixels
#define SCREEN_HEIGHT 64  // OLED display height, in pixels

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Function declarations
void handleCommand(String command);
void startWiFi();
void stopWiFi();
//void startBluetooth();
void stopBluetooth();
void scanWiFiNetworks();
void initializeDisplay();

// Variables
bool bluetoothActive = true;
String ssid = "";
String password = "";

void setup() {
  Serial.begin(115200);

  // Initialize OLED display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;
  }
  display.display();
  delay(2000);
  display.clearDisplay();
  u8g2_for_adafruit_gfx.begin(display);
  u8g2_for_adafruit_gfx.setFont(u8g2_font_baby_tf);
  // Initialize OLED display
  initializeDisplay();

  // Initialize Bluetooth
  if (!SerialBT.begin("cookiemangos")) {
    Serial.println("An error occurred initializing Bluetooth");
  } else {
    Serial.println("Bluetooth initialized");
  }
}

void loop() {
  // Check if data is available on Bluetooth
  if (SerialBT.available()) {
    String command = SerialBT.readStringUntil('\n');
    command.trim();  // Remove any leading/trailing whitespace
    handleCommand(command);
  }
}

void initializeDisplay() {
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;
  }
  display.display();
  delay(2000);
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

void handleCommand(String command) {
  Serial.println("Command received: " + command);
  SerialBT.println("Command received: " + command);d
  display.clearDisplay();
  u8g2_for_adafruit_gfx.setCursor(0, 0);
  u8g2_for_adafruit_gfx.print("Cmd: " + command);
  display.display();

  if (command.startsWith("WIFI ")) {
    int firstQuote = command.indexOf('"');
    int secondQuote = command.indexOf('"', firstQuote + 1);

    if (firstQuote != -1 && secondQuote != -1) {
      ssid = command.substring(firstQuote + 1, secondQuote);
      SerialBT.println("SSID set to: " + ssid);
      Serial.println("SSID set to: " + ssid);
      u8g2_for_adafruit_gfx.setCursor(0, 8);
      u8g2_for_adafruit_gfx.print("SSID set: " + ssid);
      display.display();
    } else {
      SerialBT.println("Invalid WIFI command format. Use: WIFI \"ssid for spaced named\"");
      Serial.println("Invalid WIFI command format. Use: WIFI \"ssid for spaced named\"");
    }
  } else if (command.startsWith("PASS ")) {
    password = command.substring(5);
    SerialBT.println("Password set to: " + password);
    Serial.println("Password set to: " + password);
    u8g2_for_adafruit_gfx.setCursor(0, 8);
    u8g2_for_adafruit_gfx.print("Password set");
    display.display();
  } else if (command == "START_WIFI") {
    startWiFi();
  } else if (command == "STOP_WIFI") {
    stopWiFi();
  } else if (command == "SCAN_WIFI") {
    scanWiFiNetworks();
  } else if (command == "BT_SCAN") {
    SerialBT.println("BT_SCAN command received, but not implemented yet.");
  } else if (command == "BT_HID") {
    SerialBT.println("BT_HID command selected....");
    Serial.println("BT_HID command selected....");
    u8g2_for_adafruit_gfx.setCursor(0, 8);
    u8g2_for_adafruit_gfx.print("BT HID Selected.");
    blueHID();
  } else if (command == "STOP_BT") {
    stopBluetooth();
  } else if (command == "CREATE_AP") {
    SerialBT.println("CREATE_AP command received, but not implemented yet.");
  } else {
    SerialBT.println("Unknown command: " + command);
    Serial.println("Unknown command: " + command);
  }
}

void startWiFi() {
  if (ssid == "" || password == "") {
    SerialBT.println("SSID or Password not set. Use WIFI and PASS commands first.");
    Serial.println("SSID or Password not set. Use WIFI and PASS commands first.");
    u8g2_for_adafruit_gfx.setCursor(0, 16);
    u8g2_for_adafruit_gfx.print("SSID/PASS not set");
    display.display();
    return;
  }

  WiFi.begin(ssid.c_str(), password.c_str());
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

void scanWiFiNetworks() {
  int numNetworks = WiFi.scanNetworks();
  display.clearDisplay();
  if (numNetworks == -1) {
    Serial.println("Failed to start WiFi scan.");
    SerialBT.println("Failed to start WiFi scan.");
    u8g2_for_adafruit_gfx.setCursor(0, 0);
    u8g2_for_adafruit_gfx.print("Scan failed");
    display.display();
    return;
  }

  SerialBT.println("Networks found:");
  u8g2_for_adafruit_gfx.setCursor(0, 0);
  u8g2_for_adafruit_gfx.print("Networks found:");
  for (int i = 0; i < numNetworks; ++i) {
    String ssid = WiFi.SSID(i);
    int32_t rssi = WiFi.RSSI(i);
    String result = "SSID: " + ssid + " RSSI: " + String(rssi) + " dBm";
    SerialBT.println(result);
    Serial.println(result);
    u8g2_for_adafruit_gfx.setCursor(0, 8 + (i * 8));
    u8g2_for_adafruit_gfx.print(result);
  }
  display.display();
  WiFi.scanDelete();  // Clear scan results
}

void stopBluetooth() {
  SerialBT.end();
  SerialBT.println("Bluetooth stopped");
  Serial.println("Bluetooth stopped");
  u8g2_for_adafruit_gfx.setCursor(0, 0);
  u8g2_for_adafruit_gfx.print("BT stopped");
  display.display();
}

void blueHID() {
  bleKeyboard.begin();
  delay(3000);
  if (bleKeyboard.isConnected()) {
    SerialBT.println("Starting BT HID....");
    Serial.println("Starting BT HID....");
    u8g2_for_adafruit_gfx.setCursor(0, 0);
    u8g2_for_adafruit_gfx.print("Starting BT HID....");
    delay(500);
    // Open Chrome
    delay(1000);
    bleKeyboard.press(KEY_LEFT_GUI);
    bleKeyboard.press('r');
    Serial.println("pressing key left gui + r");
    delay(100);
    bleKeyboard.releaseAll();
    delay(1000);
    bleKeyboard.print("chrome");
    delay(500);
    bleKeyboard.press(KEY_RETURN);
    bleKeyboard.releaseAll();
    delay(5000);  // Adjust this delay according to Chrome's load time

    // Enter URL
    bleKeyboard.print("https://www.google.com/url?sa=t&source=web&rct=j&opi=89978449&url=https://www.youtube.com/watch%3Fv%3DdQw4w9WgXcQ&ved=2ahUKEwis0KeZremGAxVCODQIHfwZA2cQtwJ6BAgPEAI&usg=AOvVaw0aHtehaphMhOCAkCydRLZU");  // Replace with the desired URL
    delay(500);
    bleKeyboard.press(KEY_RETURN);
    bleKeyboard.releaseAll();
    delay(5000);  // Adjust this delay according to page load time
                  // END

    SerialBT.println("BT HID Complete....");
    Serial.println("BT HID Complete....");
    u8g2_for_adafruit_gfx.setCursor(0, 12);
    u8g2_for_adafruit_gfx.print("BT HID Complete....!");
    delay(500);

    display.display();
  }
  else {
    Serial.println("Waiting....");
    delay(1000);
  }
}