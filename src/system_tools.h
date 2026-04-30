// system_tools.h - WiFi/AP/SD utility tools for Cypherbox V2

#ifndef SYSTEM_TOOLS_H
#define SYSTEM_TOOLS_H

#include <Arduino.h>

class SystemTools {
public:
    static bool initSd();
    static void listSdFiles();
    static void browseSdFiles();
    static void readSdFile(const String& path);
    static void deleteSdFile(const String& path, bool confirmed);
    static void startAp();
    static void stopAp();
    static void joinWiFi(const String& ssid, const String& password);
    static void disconnectWiFi();
    static void stopAll();
    static void disabledLabMode(const String& label);
};

#endif
