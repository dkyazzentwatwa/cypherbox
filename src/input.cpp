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

    bool moved = false;

    if (isButtonPressed(BUTTON_UP)) {
        if (selectedMenuItem > 0) {
            selectedMenuItem = (MenuItem)(selectedMenuItem - 1);
        } else {
            selectedMenuItem = (MenuItem)(NUM_MENU_ITEMS - 1);
        }
        moved = true;
        lastNavTime = millis();
    }

    if (isButtonPressed(BUTTON_DOWN)) {
        selectedMenuItem = (MenuItem)(selectedMenuItem + 1);
        if (selectedMenuItem >= NUM_MENU_ITEMS) {
            selectedMenuItem = (MenuItem)0;
        }
        moved = true;
        lastNavTime = millis();
    }

    if (moved) {
        int selected = (int)selectedMenuItem;
        int maxFirstVisible = max(0, (int)NUM_MENU_ITEMS - 2);
        if (selected < firstVisibleMenuItem) {
            firstVisibleMenuItem = selected;
        } else if (selected > firstVisibleMenuItem + 1) {
            firstVisibleMenuItem = selected - 1;
        }
        firstVisibleMenuItem = constrain(firstVisibleMenuItem, 0, maxFirstVisible);
    }

    if (isButtonPressed(BUTTON_SELECT)) {
        buttonPressed = true;
        lastNavTime = millis();
    }
}
