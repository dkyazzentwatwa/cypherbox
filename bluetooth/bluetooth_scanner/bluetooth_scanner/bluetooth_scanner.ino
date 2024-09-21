#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <BluetoothSerial.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

// OLED display settings
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SSD1306_I2C_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Bluetooth Classic settings
BluetoothSerial SerialBT;

// BLE settings
BLEScan* pBLEScan;
int scanTime = 5;  // BLE scan time in seconds

// Function to display text on OLED
void displayText(String text) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print(text);
  display.display();
}

// BLE Advertised Device Callbacks
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    String deviceInfo = String("BLE Device found: ") + advertisedDevice.toString().c_str();
    Serial.println(deviceInfo);
    displayText(deviceInfo);
    delay(200);
  }
};

// Function to initialize display
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


void setup() {
  Serial.begin(115200);
  //delay(10);
  initDisplay();
  // Initialize Bluetooth Classic
  if (!SerialBT.begin("ESP32_BT_Scanner")) {
    Serial.println("An error occurred initializing Bluetooth");
    displayText("BT init error");
  } else {
    Serial.println("Bluetooth initialized");
    displayText("BT initialized");
  }

  // Initialize BLE
  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan();  // Create new scan
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);  // Active scan uses more power, but gets results faster
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);  // Less or equal setInterval value

  delay(2000);
}

void loop() {
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
    displayText(deviceInfo + "\nBLE Devices: " + String(bleDeviceCount));
    delay(2000);  // Display each device info for 2 seconds
  }
  delay(30000);  // Wait before scanning again
}