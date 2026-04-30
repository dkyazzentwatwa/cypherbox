// input.cpp - Input Handling Module Implementation for Cypherbox V2

#include "input.h"

// Static members
bool Input::buttonPressed = false;
ButtonState Input::buttonStates[3] = {
    {BUTTON_UP,     HIGH, HIGH, 0, false},
    {BUTTON_DOWN,   HIGH, HIGH, 0, false},
    {BUTTON_SELECT, HIGH, HIGH, 0, false}
};

void Input::init() {
    pinMode(BUTTON_UP,     INPUT_PULLUP);
    pinMode(BUTTON_DOWN,   INPUT_PULLUP);
    pinMode(BUTTON_SELECT, INPUT_PULLUP);
    Serial.println("Input initialized");
}

bool Input::readButtonRaw(uint8_t pin) {
    return digitalRead(pin) == LOW;
}

void Input::updateButtonStates() {
    unsigned long now = millis();
    for (int i = 0; i < 3; i++) {
        bool raw = readButtonRaw(buttonStates[i].pin);
        if (raw != buttonStates[i].lastState) {
            buttonStates[i].lastChangeTime = now;
            buttonStates[i].lastState = raw;
        }
        buttonStates[i].currentState = raw;
    }
}

bool Input::isButtonPressed(uint8_t pin) {
    updateButtonStates();
    for (int i = 0; i < 3; i++) {
        if (buttonStates[i].pin == pin) {
            unsigned long now = millis();
            if (buttonStates[i].currentState &&
                (now - buttonStates[i].lastChangeTime) > DEBOUNCE_MS) {
                return true;
            }
        }
    }
    return false;
}

void Input::handleMenuSelection(MenuItem& selectedMenuItem, int& firstVisibleMenuItem) {
    // Simple debounced navigation (non-blocking)
    static unsigned long lastNavTime = 0;
    if (millis() - lastNavTime < 150) return;

    if (isButtonPressed(BUTTON_UP)) {
        if (selectedMenuItem > 0) {
            selectedMenuItem = (MenuItem)(selectedMenuItem - 1);
        } else {
            selectedMenuItem = (MenuItem)(NUM_MENU_ITEMS - 1);
        }
        if (selectedMenuItem < firstVisibleMenuItem) {
            firstVisibleMenuItem = selectedMenuItem;
        }
        lastNavTime = millis();
    }

    if (isButtonPressed(BUTTON_DOWN)) {
        selectedMenuItem = (MenuItem)(selectedMenuItem + 1);
        if (selectedMenuItem >= NUM_MENU_ITEMS) {
            selectedMenuItem = (MenuItem)0;
        }
        if (selectedMenuItem > firstVisibleMenuItem + 1) {
            firstVisibleMenuItem = selectedMenuItem - 1;
        }
        lastNavTime = millis();
    }

    if (isButtonPressed(BUTTON_SELECT)) {
        buttonPressed = true;
        lastNavTime = millis();
    }
}
