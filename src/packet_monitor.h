// packet_monitor.h - Passive packet monitor and PCAP recorder for Cypherbox V2

#ifndef PACKET_MONITOR_H
#define PACKET_MONITOR_H

#include <Arduino.h>
#include <esp_wifi.h>

class PacketMonitor {
public:
    static void init();
    static void runMonitor(bool verboseSniffer = false);
    static void stop();
    static void setChannel(uint8_t channel);
    static void startRecording();
    static void stopRecording();
    static void toggleRecording(bool enabled);
    static void showStatus();
    static void showRawFiles();
    static void flushBuffer();
    static int getAverageRssi() { return averageRssi; }
    static uint32_t getPacketCount() { return packetCount; }
    static bool isRecording() { return recording; }

private:
    static void IRAM_ATTR onPacket(void* buf, wifi_promiscuous_pkt_type_t type);
    static void drawStatus();
    static void ensureBuffer();

    static bool running;
    static bool recording;
    static bool verbose;
    static uint8_t currentChannel;
    static volatile uint32_t packetCount;
    static volatile uint32_t managementCount;
    static volatile uint32_t dataCount;
    static volatile uint32_t controlCount;
    static volatile int rssiSum;
    static volatile uint32_t rssiSamples;
    static int averageRssi;
};

#endif
