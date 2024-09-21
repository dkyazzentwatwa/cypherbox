#include "DisplayManager.h"

// Screen Setup
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

/**
 * Initializes the display by initializing the SSD1306 OLED display with the
 * given parameters. If the initialization fails, it prints an error message
 * and enters an infinite loop. After initialization, it clears the display
 * and updates it.
 *
 * @throws None
 */
void initDisplay() {
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println(F("SSD1306 allocation failed"));
        for (;;);
    }
    display.clearDisplay();
    display.display();
}

void drawMenu(const char** menuLabels, int numMenuItems, int selectedMenuItem, int firstVisibleMenuItem) {
    display.clearDisplay();

    for (int i = 0; i < MAX_VISIBLE_MENU_ITEMS; i++) {
        int menuIndex = (firstVisibleMenuItem + i) % numMenuItems;
        int16_t x = 5;
        int16_t y = 10 + (i * 20);  // Adjust vertical spacing as needed

        // Highlight the selected item
        if (selectedMenuItem == menuIndex) {
            display.fillRect(0, y - 2, SCREEN_WIDTH, 15, SSD1306_WHITE);
            display.setTextColor(SSD1306_BLACK);
        } else {
            display.setTextColor(SSD1306_WHITE);
        }

        // Draw menu item text
        display.setCursor(x, y);
        display.setTextSize(1);
        display.println(menuLabels[menuIndex]);
    }

    display.display();
    updateNeoPixel(); // Ensure this function is defined if used
}



/*

 * Updates the display based on the current menu and selected menu items.
 *
 * @param currentMenu The current menu being displayed.
 * @param selectedMainMenuItem The selected main menu item.
 * @param selectedPacketMonitorMenuItem The selected packet monitor menu item.
 * @param selectedWiFiSniffMenuItem The selected WiFi sniffer menu item.
 *
 * @throws None
 
void updateDisplay(CurrentMenu currentMenu, MainMenuItem selectedMainMenuItem, PacketMonitorMenuItem selectedPacketMonitorMenuItem, WiFiSniffMenuItem selectedWiFiSniffMenuItem) {
    display.clearDisplay();
    switch (currentMenu) {
        case MAIN_MENU:
            // Display main menu
            break;
        case PACKET_MON_MENU:
            // Display packet monitor menu
            break;
        case WIFI_SNIFF_MENU:
            // Display WiFi sniffer menu
            break;
        default:
            break;
    }
    display.display();
}
*/
