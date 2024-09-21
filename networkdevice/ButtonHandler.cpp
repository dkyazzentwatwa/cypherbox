#include "ButtonHandler.h"
#include <Arduino.h>

// Buttons
#define UP_BUTTON_PIN 35
#define DOWN_BUTTON_PIN 34
#define SELECT_BUTTON_PIN 15
#define HOME_BUTTON_PIN 2


/**
 * Initializes the buttons by setting their pin mode to input with pull-up resistor.
 *
 * @throws None
 */
void initButtons() {
    pinMode(UP_BUTTON_PIN, INPUT_PULLUP);
    pinMode(DOWN_BUTTON_PIN, INPUT_PULLUP);
    pinMode(SELECT_BUTTON_PIN, INPUT_PULLUP);
    pinMode(HOME_BUTTON_PIN, INPUT_PULLUP);
}

    /**
     * Handles button presses based on the current state of the buttons.
     *
     * This function checks the state of the up, down, select, and home buttons.
     * If the up button is pressed, it handles the up button press.
     * If the down button is pressed, it handles the down button press.
     * If the select button is pressed, it checks the current menu and calls the appropriate function to handle the select button press.
     * If the home button is pressed, it sets the current menu to the main menu.
     *
     * @throws None
     */
void handleButtonPresses() {
    if (digitalRead(UP_BUTTON_PIN) == LOW) {
        // Handle up button press
    } else if (digitalRead(DOWN_BUTTON_PIN) == LOW) {
        // Handle down button press
    } else if (digitalRead(SELECT_BUTTON_PIN) == LOW) {
        // Handle select button press
        if (currentMenu == MAIN_MENU) {
            selectMainMenuItem(selectedMainMenuItem);
        } else if (currentMenu == PACKET_MON_MENU) {
            selectPacketMonitorMenuItem(selectedPacketMonitorMenuItem);
        } else if (currentMenu == WIFI_SNIFF_MENU) {
            selectWiFiSniffMenuItem(selectedWiFiSniffMenuItem);
        }
    } else if (digitalRead(HOME_BUTTON_PIN) == LOW) {
        // Handle home button press to return to main menu
        currentMenu = MAIN_MENU;
    }
}