#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define RST_PIN         25
#define SS_PIN          27
#define SCREEN_WIDTH    128
#define SCREEN_HEIGHT   64
#define OLED_RESET      -1
#define SSD1306_I2C_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
MFRC522 mfrc522(SS_PIN, RST_PIN);

void initDisplay() {
  if (!display.begin(SSD1306_SWITCHCAPVCC, SSD1306_I2C_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
  display.display();
  delay(2000);
  display.clearDisplay();
}

void drawBorder() {
  display.drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_WHITE);
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
  
  // Info lines
  display.setCursor(4, 18);
  display.println(info1);
  display.setCursor(4, 28);
  display.println(info2);
  display.setCursor(4, 38);
  display.println(info3);
  display.display();
}

void setup() {
  Serial.begin(115200);
  delay(5000);
  Serial.println("MFRC522 NFC Reader");
  
  initDisplay();
  displayInfo("MFRC522 NFC Reader", "Initializing...");
  
  SPI.begin();
  mfrc522.PCD_Init();
  delay(4);
  mfrc522.PCD_DumpVersionToSerial();
  
  String fwVersion = "FW: " + String(mfrc522.PCD_ReadRegister(mfrc522.VersionReg), HEX);
  String chipInfo = "Chip: MFRC522";
  
  displayInfo("MFRC522 Info", chipInfo, fwVersion);
  delay(2000);
  
  displayInfo("Ready", "Waiting for card...", "Place card near", "the reader");
}

void loop() {
  if (!mfrc522.PICC_IsNewCardPresent()) {
    // No card found, update display every 5 seconds
    static unsigned long lastUpdate = 0;
    if (millis() - lastUpdate > 5000) {
      displayInfo("Ready", "Waiting for card...", "Place card near", "the reader");
      lastUpdate = millis();
    }
    return;
  }

  if (!mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  String uidString = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
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