#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define RST_PIN 25
#define SS_PIN 27
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SSD1306_I2C_ADDRESS 0x3C

// Buttons
#define UP_BUTTON_PIN 34
#define DOWN_BUTTON_PIN 35
#define SELECT_BUTTON_PIN 15
#define HOME_BUTTON_PIN 2

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
MFRC522 mfrc522(SS_PIN, RST_PIN);

enum MenuState {
  MENU_MAIN,
  MENU_READ,
  MENU_WRITE,
  MENU_ERASE
};

MenuState currentState = MENU_MAIN;
int selectedMenuItem = 0;
const int numMenuItems = 3;
const char* menuItems[] = { "Read", "Write", "Erase" };

void initDisplay() {
  if (!display.begin(SSD1306_SWITCHCAPVCC, SSD1306_I2C_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;
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

void displayMenu() {
  display.clearDisplay();
  drawBorder();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(4, 4);
  display.println("RFID Menu");
  display.drawLine(0, 14, SCREEN_WIDTH, 14, SSD1306_WHITE);

  for (int i = 0; i < numMenuItems; i++) {
    display.setCursor(10, 18 + i * 10);
    if (i == selectedMenuItem) {
      display.print("> ");
    } else {
      display.print("  ");
    }
    display.println(menuItems[i]);
  }

  display.display();
}

void handleRFIDButtons() {
  static unsigned long lastDebounceTime = 0;
  const unsigned long debounceDelay = 200;

  if (millis() - lastDebounceTime > debounceDelay) {
    if (digitalRead(UP_BUTTON_PIN) == LOW) {
      selectedMenuItem = (selectedMenuItem - 1 + numMenuItems) % numMenuItems;
      displayMenu();
      lastDebounceTime = millis();
    } else if (digitalRead(DOWN_BUTTON_PIN) == LOW) {
      selectedMenuItem = (selectedMenuItem + 1) % numMenuItems;
      displayMenu();
      lastDebounceTime = millis();
    } else if (digitalRead(SELECT_BUTTON_PIN) == LOW) {
      switch (selectedMenuItem) {
        case 0:
          currentState = MENU_READ;
          break;
        case 1:
          currentState = MENU_WRITE;
          break;
        case 2:
          currentState = MENU_ERASE;
          break;
      }
      lastDebounceTime = millis();
    } else if (digitalRead(HOME_BUTTON_PIN) == LOW) {
      currentState = MENU_MAIN;
      displayMenu();
      lastDebounceTime = millis();
    }
  }
}

void readRFID() {
  displayInfo("Reading RFID", "Place card near", "the reader");
  unsigned long lastUpdate = 0;
  bool cardRead = false;

  while (currentState == MENU_READ) {
    if (!cardRead && mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
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
      
      mfrc522.PICC_HaltA();
      mfrc522.PCD_StopCrypto1();
      
      cardRead = true;
      lastUpdate = millis();
    }
    
    if (cardRead && (millis() - lastUpdate > 5000)) {
      displayInfo("Read Complete", "Press HOME to", "return to menu", "or place new card");
      cardRead = false;
    }
    
    if (!cardRead && (millis() - lastUpdate > 3000)) {
      displayInfo("Reading RFID", "Place card near", "the reader");
      lastUpdate = millis();
    }
    
    if (digitalRead(HOME_BUTTON_PIN) == LOW) {
      delay(50); // Simple debounce
      if (digitalRead(HOME_BUTTON_PIN) == LOW) {
        currentState = MENU_MAIN;
      }
    }
  }
}

void writeRFID() {
  displayInfo("Writing RFID", "Place card near", "the reader");

  while (currentState == MENU_WRITE) {
    if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
      MFRC522::MIFARE_Key key;
      for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;

      byte buffer[16] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F };
      byte block = 4;
      MFRC522::StatusCode status;

      status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &key, &(mfrc522.uid));
      if (status != MFRC522::STATUS_OK) {
        displayInfo("Write Failed", "Authentication error");
        delay(2000);
        continue;
      }

      status = mfrc522.MIFARE_Write(block, buffer, 16);
      if (status != MFRC522::STATUS_OK) {
        displayInfo("Write Failed", "Writing error");
        delay(2000);
        continue;
      }

      displayInfo("Write Successful", "Data written", "to block " + String(block));
      delay(3000);

      mfrc522.PICC_HaltA();
      mfrc522.PCD_StopCrypto1();

      displayInfo("Write Complete", "Press HOME to", "return to menu");
    }

    if (digitalRead(HOME_BUTTON_PIN) == LOW) {
      delay(50);  // Simple debounce
      if (digitalRead(HOME_BUTTON_PIN) == LOW) {
        currentState = MENU_MAIN;
      }
    }
  }
}

void eraseRFID() {
  displayInfo("Erasing RFID", "Place card near", "the reader");

  while (currentState == MENU_ERASE) {
    if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
      MFRC522::MIFARE_Key key;
      for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;

      byte buffer[16] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
      byte block = 4;
      MFRC522::StatusCode status;

      status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &key, &(mfrc522.uid));
      if (status != MFRC522::STATUS_OK) {
        displayInfo("Erase Failed", "Authentication error");
        delay(2000);
        continue;
      }

      status = mfrc522.MIFARE_Write(block, buffer, 16);
      if (status != MFRC522::STATUS_OK) {
        displayInfo("Erase Failed", "Writing error");
        delay(2000);
        continue;
      }

      displayInfo("Erase Successful", "Data erased", "from block " + String(block));
      delay(3000);

      mfrc522.PICC_HaltA();
      mfrc522.PCD_StopCrypto1();

      displayInfo("Erase Complete", "Press HOME to", "return to menu");
    }

    if (digitalRead(HOME_BUTTON_PIN) == LOW) {
      delay(50);  // Simple debounce
      if (digitalRead(HOME_BUTTON_PIN) == LOW) {
        currentState = MENU_MAIN;
      }
    }
  }
}

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;  // Wait for serial port to connect

  SPI.begin();
  mfrc522.PCD_Init();
  Wire.begin();

  initDisplay();
  displayInfo("MFRC522 NFC Reader", "Initializing...");

  delay(4000);
  //mfrc522.PCD_DumpVersionToSerial();

  String fwVersion = "FW: " + String(mfrc522.PCD_ReadRegister(mfrc522.VersionReg), HEX);
  String chipInfo = "Chip: MFRC522";

  displayInfo("MFRC522 Info", chipInfo, fwVersion);
  delay(2000);

  pinMode(UP_BUTTON_PIN, INPUT_PULLUP);
  pinMode(DOWN_BUTTON_PIN, INPUT_PULLUP);
  pinMode(SELECT_BUTTON_PIN, INPUT_PULLUP);
  pinMode(HOME_BUTTON_PIN, INPUT_PULLUP);

  displayMenu();
}

void loop() {
  handleRFIDButtons();

  switch (currentState) {
    case MENU_MAIN:
      displayMenu();
      break;
    case MENU_READ:
      readRFID();
      break;
    case MENU_WRITE:
      writeRFID();
      break;
    case MENU_ERASE:
      eraseRFID();
      break;
  }
}