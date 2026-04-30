// cypherbox_webserver.h - Web Server with Captive Portal for Cypherbox V2

#ifndef CYPHERBOX_WEBSERVER_H
#define CYPHERBOX_WEBSERVER_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>

class StarbeamWebServer {
public:
    static void init();
    static void start();
    static void stop();
    static void handleClient();
    static bool isRunning();
    static String getIP();
    static void setWifiScanEnabled(bool enabled) { wifiScanEnabled = enabled; }
    static void setBleScanEnabled(bool enabled) { bleScanEnabled = enabled; }
    static bool isWifiScanEnabled() { return wifiScanEnabled; }
    static bool isBleScanEnabled() { return bleScanEnabled; }

private:
    static void handleRoot();
    static void handleWiFi();
    static void handleBLE();
    static void handleWiFiJSON();
    static void handleBLEJSON();
    static void handleNotFound();
    static void handleScanControl();
    static void handleStatus();
    static void handleSecurityRoot();
    static String generateHTMLHeader(const char* title);
    static String generateHTMLFooter();
    static String generateWiFiTable();
    static String generateBLETable();
    static String generateNavigation();
    static String getSecurityString(uint8_t authMode);

    static WebServer* server;
    static DNSServer* dnsServer;
    static bool running;
    static IPAddress apIP;
    static bool wifiScanEnabled;
    static bool bleScanEnabled;
};

#endif
