  #include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <RTClib.h>

// Define the screen dimensions
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// Create an SSD1306 display object
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Create an RTC object
RTC_Millis rtc;

void setup() {
  // Initialize serial communication for debugging
  Serial.begin(115200);

  // Initialize the display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
  display.display();
  delay(2000);
  display.clearDisplay();

  // Initialize the RTC
  rtc.begin(DateTime(F(__DATE__), F(__TIME__)));

  // Optionally, set the time manually
  // DateTime manualTime = DateTime(2024, 6, 18, 12, 0, 0);
  // rtc.adjust(manualTime);
}

void loop() {
  // Get the current time
  DateTime now = rtc.now();

  // Clear the display buffer
  display.clearDisplay();

  // Display the time on the OLED screen
  display.setTextSize(1); // Set text size
  display.setTextColor(SSD1306_WHITE); // Set text color

  // Create a buffer to store the formatted time string
  char timeString[10];
  snprintf(timeString, sizeof(timeString), "%02d:%02d:%02d", now.hour(), now.minute(), now.second());

  // Set cursor position
  display.setCursor(0, 0);
  display.print("The time is: ");
  display.setCursor(0, 20);
  display.setTextSize(2); // Set text size

  // Print the time to the display
  display.print(timeString);

  // Update the display with the new information
  display.display();

  // Wait for a second before updating the time again
  delay(1000);
}