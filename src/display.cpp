// display.cpp - Display Module Implementation for Cypherbox V2
// Adapted from starbeam_v2

#include "display.h"
#include "terminal.h"

// Static member initialization
Adafruit_SSD1306 Display::oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
U8G2_FOR_ADAFRUIT_GFX Display::u8g2;

// Menu labels — must match MenuItem enum order in types.h
const char* Display::menuLabels[NUM_MENU_ITEMS] = {
    // Cypherbox Original
    "Packet Monitor",
    "WiFi Sniffer",
    "AP Scan",
    "AP Join",
    "AP Create",
    "Stop AP",
    "Stop Server",
    "BT Scan",
    "BT Create",
    "BT Serial Cmd",
    "BT HID Test",
    "Lab Mode Off",
    "RFID",
    "Read Blocks",
    "Wardriver",
    "Party Light",
    "Light Off",
    "Files",
    "Read Files",
    // Starbeam V2 Modules
    "WiFi Scanner",
    "WiFi Heatmap",
    "BLE Scanner",
    "Web Server ON",
    "Web Server OFF",
    "Web Status",
    // Security Testing
    "Deauth Target",
    "Deauth Broadcast",
    "Beacon Flood",
    "Probe Flood",
    "PMKID Capture",
    // Recording & Utility
    "Rec Raw",
    "Play Raw",
    "Show Raw",
    "Show Buffer",
    "Get RSSI",
    "Flush Buffer",
    "Stop All",
    "Settings",
    "Help",
    // Captive Portal
    "Captive Portal",
    "Stop C-Portal",
    "C-Portal Status"
};

void Display::init() {
    if (!oled.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDR)) {
        Serial.println(F("SSD1306 allocation failed"));
        for (;;);
    }
    u8g2.begin(oled);
    oled.display();
    delay(2000);
    oled.clearDisplay();
}

void Display::drawMenu(MenuItem selectedMenuItem, int firstVisibleMenuItem) {
    oled.clearDisplay();
    oled.setTextColor(SSD1306_WHITE);
    u8g2.setFont(u8g2_font_baby_tf);

    // Title bar
    oled.fillRect(0, 0, SCREEN_WIDTH, 16, SSD1306_WHITE);
    oled.setTextColor(SSD1306_BLACK);
    oled.setCursor(5, 4);
    oled.setTextSize(1);
    oled.println("Home");

    // Menu items (2 at a time)
    oled.setTextColor(SSD1306_WHITE);
    for (int i = 0; i < 2; i++) {
        int menuIndex = (firstVisibleMenuItem + i) % NUM_MENU_ITEMS;
        int16_t x = 5;
        int16_t y = 20 + (i * 20);

        if (selectedMenuItem == menuIndex) {
            oled.fillRect(0, y - 2, SCREEN_WIDTH, 15, SSD1306_WHITE);
            oled.setTextColor(SSD1306_BLACK);
        } else {
            oled.setTextColor(SSD1306_WHITE);
        }

        oled.setCursor(x, y);
        oled.setTextSize(1);
        oled.println(menuLabels[menuIndex]);
    }

    oled.display();

    // Echo to serial
    Terminal::echoToSerial(
        "MENU",
        String(selectedMenuItem > 0 ? "> " : "") + menuLabels[selectedMenuItem],
        "[UP/DOWN/SELECT]",
        ""
    );
}

void Display::drawBorder() {
    oled.drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_WHITE);
}

void Display::displayInfo(String title, String info1, String info2, String info3) {
    oled.clearDisplay();
    drawBorder();
    oled.setTextSize(1);
    oled.setTextColor(SSD1306_WHITE);
    oled.setCursor(4, 4);
    oled.println(title);
    oled.drawLine(0, 14, SCREEN_WIDTH, 14, SSD1306_WHITE);
    oled.setCursor(4, 18);
    oled.println(info1);
    oled.setCursor(4, 28);
    oled.println(info2);
    oled.setCursor(4, 38);
    oled.println(info3);
    oled.display();
    Terminal::echoToSerial(title, info1, info2, info3);
}

void Display::updateDisplay(const char* message) {
    oled.clearDisplay();
    oled.setTextSize(1);
    oled.setTextColor(SSD1306_WHITE);
    oled.setCursor(0, 0);
    oled.print(message);
    oled.display();
    Terminal::echoToSerial(String(message), "", "", "");
}

void Display::displayTitleScreen() {
    oled.clearDisplay();
    u8g2.setFont(u8g2_font_adventurer_tr);
    u8g2.setCursor(20, 40);
    u8g2.print("CYPHERBOX");
    oled.display();
}

void Display::displayInfoScreen() {
    oled.clearDisplay();
    u8g2.setFont(u8g2_font_baby_tf);
    u8g2.setCursor(0, 22);
    u8g2.print("Welcome to CYPHERBOX!");
    u8g2.setCursor(0, 30);
    u8g2.print("ESP32 cybersecurity toolkit.");
    u8g2.setCursor(0, 38);
    u8g2.print("WiFi. BLE. RFID. GPS.");
    u8g2.setCursor(0, 46);
    u8g2.print("SD Card + OLED UI.");
    u8g2.setCursor(0, 54);
    u8g2.print("Be safe. Be smart.");
    oled.display();
}

void Display::displayLegalWarning() {
    oled.clearDisplay();
    oled.setTextSize(1);
    oled.setTextColor(SSD1306_WHITE);
    oled.setCursor(0, 0);
    oled.println("LEGAL WARNING");
    oled.println("");
    oled.println("Security testing");
    oled.println("features are for");
    oled.println("AUTHORIZED use");
    oled.println("ONLY.");
    oled.println("");
    oled.println("[SELECT] Accept");
    oled.display();
}
