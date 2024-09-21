#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Adafruit_SSD1306.h>
#include <U8g2_for_Adafruit_GFX.h>

// Function declarations
void initDisplay();
void drawMenu(const char** menuLabels, int numMenuItems, int selectedMenuItem, int firstVisibleMenuItem);
void updateDisplay(CurrentMenu currentMenu, MainMenuItem selectedMainMenuItem, PacketMonitorMenuItem selectedPacketMonitorMenuItem, WiFiSniffMenuItem selectedWiFiSniffMenuItem /* add more menu items here */);

#endif // DISPLAY_MANAGER_H