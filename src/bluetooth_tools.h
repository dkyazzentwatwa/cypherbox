// bluetooth_tools.h - Bluetooth helper modes for Cypherbox V2

#ifndef BLUETOOTH_TOOLS_H
#define BLUETOOTH_TOOLS_H

#include <Arduino.h>

class BluetoothTools {
public:
    static void startSerial();
    static void runSerialBridge();
    static void stop();
    static void runHidSafeTest();
    static bool isSerialRunning() { return serialRunning; }

private:
    static void handleSerialCommand(const String& command);
    static bool serialRunning;
};

#endif
