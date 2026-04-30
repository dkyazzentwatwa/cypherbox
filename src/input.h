// input.h - Input Handling Module for Cypherbox V2
// Non-blocking button handling

#ifndef INPUT_H
#define INPUT_H

#include <Arduino.h>
#include "../config.h"
#include "../types.h"

class Input {
public:
    static void init();
    static bool isButtonPressed(uint8_t pin);
    static void handleMenuSelection(MenuItem& selectedMenuItem, int& firstVisibleMenuItem);
    static bool getButtonPressedState() { return buttonPressed; }
    static void setButtonPressedState(bool state) { buttonPressed = state; }

private:
    static bool buttonPressed;
    static ButtonState buttonStates[3];
    static void updateButtonStates();
    static bool readButtonRaw(uint8_t pin);
};

#endif // INPUT_H
