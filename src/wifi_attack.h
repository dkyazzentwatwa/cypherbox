// src/wifi_attack.h - WiFi Attack Module for Cypherbox V2

#ifndef WIFI_ATTACK_H
#define WIFI_ATTACK_H

#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include "../config.h"

#define ATTACK_DEAUTH_TARGETED  0
#define ATTACK_DEAUTH_BROADCAST 1
#define ATTACK_BEACON_FLOOD     2
#define ATTACK_PROBE_FLOOD      3
#define ATTACK_PMKID_CAPTURE    4

typedef struct {
  uint8_t frame_control[2];
  uint8_t duration[2];
  uint8_t station[6];
  uint8_t sender[6];
  uint8_t access_point[6];
  uint8_t fragment_sequence[2];
  uint16_t reason;
} __attribute__((packed)) deauth_frame_t;

typedef struct {
  uint8_t frame_control[2];
  uint8_t duration[2];
  uint8_t destination[6];
  uint8_t source[6];
  uint8_t bssid[6];
  uint8_t sequence[2];
} __attribute__((packed)) beacon_header_t;

class WiFiAttack {
public:
    static void init();
    static void deinit();
    static void startDeauthTargeted(int wifi_index, uint16_t reason);
    static void startDeauthBroadcast(uint16_t reason);
    static void startBeaconFlood(const char** ssid_list, int count);
    static void startProbeFlood(const char* target_ssid);
    static void startPMKIDCapture(int wifi_index);
    static void stopAttack();
    static void channelHop();
    static bool isAttacking() { return attacking; }
    static int getAttackType() { return currentAttackType; }
    static int getFramesSent() { return framesSent; }
    static void handleAttackLoop();

private:
    static void IRAM_ATTR deauthSniffer(void* buf, wifi_promiscuous_pkt_type_t type);
    static void IRAM_ATTR pmkidSniffer(void* buf, wifi_promiscuous_pkt_type_t type);
    static void buildDeauthFrame(uint8_t* station, uint8_t* ap, uint16_t reason);
    static void setupWiFiForAttack(uint8_t channel);
    static void enablePromiscuous();
    static void disablePromiscuous();

    static bool attacking;
    static int currentAttackType;
    static int framesSent;
    static int currentChannel;
    static deauth_frame_t deauthFrame;
    static unsigned long attackStartTime;
    static char beaconSSIDs[MAX_BEACON_SSIDS][33];
    static uint8_t beaconBSSID[6];
    static int beaconSSIDCount;
    static char probeTargetSSID[33];
    static bool pmkidFound;
    static char pmkidSSID[33];
    static uint8_t pmkidAPMac[6];
    static uint8_t pmkidData[16];
};

#endif
