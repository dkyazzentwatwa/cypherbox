// display.h - Display Module for Cypherbox V2
// Adapted from starbeam_v2

#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <U8g2_for_Adafruit_GFX.h>
#include "../config.h"
#include "../types.h"

class Display {
public:
    static void init();
    static void drawMenu(MenuItem selectedMenuItem, int firstVisibleMenuItem);
    static void displayInfo(String title, String info1 = "", String info2 = "", String info3 = "");
    static void updateDisplay(const char* message);
    static void displayTitleScreen();
    static void displayInfoScreen();
    static void displayLegalWarning();
    static void drawBorder();
    // Expose display objects for other modules
    static Adafruit_SSD1306& getOled() { return oled; }
    static U8G2_FOR_ADAFRUIT_GFX& getU8g2() { return u8g2; }

private:
    static Adafruit_SSD1306 oled;
    static U8G2_FOR_ADAFRUIT_GFX u8g2;
    static const char* menuLabels[NUM_MENU_ITEMS];
};

#endif // DISPLAY_H
