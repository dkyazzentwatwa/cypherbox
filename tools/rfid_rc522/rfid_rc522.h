#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// OLED display settings
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SSD1306_I2C_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define RST_PIN  0   // Alternative RST-PIN for RC522 - RFID - SPI - Module GPIO4
#define SS_PIN   5   // Alternative SDA-PIN for RC522 - RFID - SPI - Module GPIO5

MFRC522 rfid(SS_PIN, RST_PIN); // Create MFRC522 instance

byte buffer[16][18];  // Buffer to store read data from 16 blocks
MFRC522::MIFARE_Key key;

void initDisplay() {
  if (!display.begin(SSD1306_SWITCHCAPVCC, SSD1306_I2C_ADDRESS)) {  // Address 0x3C for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
  display.display();
  delay(2000);
  display.clearDisplay();
}

void displayText(String text) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print(text);
  display.display();
}

void setup() {
  Serial.begin(115200);
  delay(10);
  initDisplay();
  SPI.begin();  // Init SPI bus
  rfid.PCD_Init();  // Init MFRC522
  displayText("Place source card to read from...");
  Serial.println("Place your source card to read from...");

  // Prepare key - all keys are set to FFFFFFFFFFFFh at chip delivery from the factory
  for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;
}

void loop() {
  // Look for new cards
  if (!rfid.PICC_IsNewCardPresent()) return;

  // Select one of the cards
  if (!rfid.PICC_ReadCardSerial()) return;

  Serial.print("UID tag: ");
  String uidText = "UID: ";
  for (byte i = 0; i < rfid.uid.size; i++) {
    Serial.print(rfid.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(rfid.uid.uidByte[i], HEX);
    uidText += String(rfid.uid.uidByte[i] < 0x10 ? " 0" : " ");
    uidText += String(rfid.uid.uidByte[i], HEX);
  }
  Serial.println();
  displayText(uidText);

  // Read data from the first 16 blocks
  for (byte block = 0; block < 16; block++) {
    readBlock(block, buffer[block]);
  }

  // Halt PICC
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();

  // Indicate reading is done
  displayText("Reading completed.");
  Serial.println("Reading from source card completed.");
  delay(2000); // Wait for 2 seconds before proceeding to write to the target card

  // Write the data to the target card
  displayText("Place target card to write to...");
  Serial.println("Place your target card to write to...");
  while (!rfid.PICC_IsNewCardPresent());
  if (!rfid.PICC_ReadCardSerial()) return;
  for (byte block = 0; block < 16; block++) {
    writeBlock(block, buffer[block]);
  }

  // Indicate writing is done
  displayText("Writing completed.");
  Serial.println("Writing to target card completed.");

  // Halt PICC
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}

void readBlock(byte blockAddr, byte *buffer) {
  MFRC522::StatusCode status;
  byte size = 18;

  // Authenticate using key A
  status = rfid.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, blockAddr, &key, &(rfid.uid));
  if (status != MFRC522::STATUS_OK) {
    String errorText = "Auth failed blk ";
    errorText += String(blockAddr);
    displayText(errorText);
    Serial.print("Authentication failed for block ");
    Serial.print(blockAddr);
    Serial.print(": ");
    Serial.println(rfid.GetStatusCodeName(status));
    return;
  }

  // Read data from the block
  status = rfid.MIFARE_Read(blockAddr, buffer, &size);
  if (status != MFRC522::STATUS_OK) {
    String errorText = "Read failed blk ";
    errorText += String(blockAddr);
    displayText(errorText);
    Serial.print("Reading failed for block ");
    Serial.print(blockAddr);
    Serial.print(": ");
    Serial.println(rfid.GetStatusCodeName(status));
    return;
  }

  Serial.print("Data in block ");
  Serial.print(blockAddr);
  Serial.print(": ");
  String blockText = "Block ";
  blockText += String(blockAddr);
  blockText += ": ";
  for (byte i = 0; i < 16; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
    blockText += String(buffer[i] < 0x10 ? " 0" : " ");
    blockText += String(buffer[i], HEX);
  }
  Serial.println();
  displayText(blockText);
}

void writeBlock(byte blockAddr, byte *buffer) {
  MFRC522::StatusCode status;

  // Authenticate using key A
  status = rfid.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, blockAddr, &key, &(rfid.uid));
  if (status != MFRC522::STATUS_OK) {
    String errorText = "Auth failed blk ";
    errorText += String(blockAddr);
    displayText(errorText);
    Serial.print("Authentication failed for block ");
    Serial.print(blockAddr);
    Serial.print(": ");
    Serial.println(rfid.GetStatusCodeName(status));
    return;
  }

  // Write data to the block
  status = rfid.MIFARE_Write(blockAddr, buffer, 16);
  if (status != MFRC522::STATUS_OK) {
    String errorText = "Write failed blk ";
    errorText += String(blockAddr);
    displayText(errorText);
    Serial.print("Writing failed for block ");
    Serial.print(blockAddr);
    Serial.print(": ");
    Serial.println(rfid.GetStatusCodeName(status));
    return;
  }

  Serial.print("Data written to block ");
  Serial.println(blockAddr);
  String successText = "Written to blk ";
  successText += String(blockAddr);
  displayText(successText);
}