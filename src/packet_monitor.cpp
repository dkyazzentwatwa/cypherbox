// packet_monitor.cpp - Passive packet monitor and PCAP recorder for Cypherbox V2

#include "packet_monitor.h"
#include "Buffer.h"
#include "display.h"
#include "input.h"
#include <WiFi.h>
#include <esp_wifi.h>
#include <SD.h>
#include "../config.h"

static Buffer* monitorBuffer = nullptr;

bool PacketMonitor::running = false;
bool PacketMonitor::recording = false;
bool PacketMonitor::verbose = false;
uint8_t PacketMonitor::currentChannel = 1;
volatile uint32_t PacketMonitor::packetCount = 0;
volatile uint32_t PacketMonitor::managementCount = 0;
volatile uint32_t PacketMonitor::dataCount = 0;
volatile uint32_t PacketMonitor::controlCount = 0;
volatile int PacketMonitor::rssiSum = 0;
volatile uint32_t PacketMonitor::rssiSamples = 0;
int PacketMonitor::averageRssi = 0;

void PacketMonitor::ensureBuffer() {
    if (!monitorBuffer) monitorBuffer = new Buffer();
}

void PacketMonitor::init() {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);
    esp_wifi_set_promiscuous(false);
    esp_wifi_set_channel(currentChannel, WIFI_SECOND_CHAN_NONE);
    esp_wifi_set_promiscuous_rx_cb(&PacketMonitor::onPacket);
    packetCount = 0;
    managementCount = 0;
    dataCount = 0;
    controlCount = 0;
    rssiSum = 0;
    rssiSamples = 0;
    running = true;
    esp_wifi_set_promiscuous(true);
}

void PacketMonitor::setChannel(uint8_t channel) {
    if (channel < 1 || channel > 13) channel = 1;
    currentChannel = channel;
    esp_wifi_set_channel(currentChannel, WIFI_SECOND_CHAN_NONE);
    Serial.printf("Packet monitor channel set to %u\n", currentChannel);
}

void PacketMonitor::startRecording() {
    if (recording) return;
    if (!SD.begin(SD_CS)) {
        Serial.println("Packet record: SD init failed");
        Display::displayInfo("Packet Record", "SD init failed", "", "");
        return;
    }
    ensureBuffer();
    monitorBuffer->open(&SD);
    recording = true;
    Serial.println("Packet PCAP recording ON");
}

void PacketMonitor::stopRecording() {
    if (!recording) return;
    if (monitorBuffer) monitorBuffer->close(&SD);
    recording = false;
    Serial.println("Packet PCAP recording OFF");
}

void PacketMonitor::toggleRecording(bool enabled) {
    if (enabled) startRecording();
    else stopRecording();
}

void IRAM_ATTR PacketMonitor::onPacket(void* buf, wifi_promiscuous_pkt_type_t type) {
    wifi_promiscuous_pkt_t* pkt = (wifi_promiscuous_pkt_t*)buf;
    if (!pkt || type == WIFI_PKT_MISC) return;
    uint32_t length = pkt->rx_ctrl.sig_len;
    if (length > SNAP_LEN) return;
    packetCount++;
    if (type == WIFI_PKT_MGMT) managementCount++;
    else if (type == WIFI_PKT_DATA) dataCount++;
    else if (type == WIFI_PKT_CTRL) controlCount++;
    rssiSum += pkt->rx_ctrl.rssi;
    rssiSamples++;
    if (recording && monitorBuffer) {
        monitorBuffer->addPacket(pkt->payload, length);
    }
}

void PacketMonitor::drawStatus() {
    if (rssiSamples > 0) averageRssi = rssiSum / (int)rssiSamples;
    Display::displayInfo(verbose ? "WiFi Sniffer" : "Packet Monitor",
                         "CH " + String(currentChannel) + " RSSI " + String(averageRssi),
                         "Pkt " + String(packetCount) + " M " + String(managementCount),
                         String(recording ? "REC " : "") + "UP/DN ch SEL exit");
}

void PacketMonitor::runMonitor(bool verboseSniffer) {
    verbose = verboseSniffer;
    init();
    drawStatus();
    delay(250);
    unsigned long lastDraw = millis();
    while (true) {
        if (Input::isButtonPressed(BUTTON_UP)) {
            setChannel(currentChannel == 1 ? 13 : currentChannel - 1);
            delay(180);
        }
        if (Input::isButtonPressed(BUTTON_DOWN)) {
            setChannel(currentChannel == 13 ? 1 : currentChannel + 1);
            delay(180);
        }
        if (Input::isButtonPressed(BUTTON_SELECT)) {
            delay(200);
            stop();
            return;
        }
        if (millis() - lastDraw > 750) {
            drawStatus();
            if (verbose) {
                Serial.printf("ch:%u packets:%lu mgmt:%lu data:%lu ctrl:%lu rssi:%d rec:%s\n",
                              currentChannel, packetCount, managementCount, dataCount,
                              controlCount, averageRssi, recording ? "on" : "off");
            }
            lastDraw = millis();
        }
        if (recording && monitorBuffer) monitorBuffer->save(&SD);
        yield();
    }
}

void PacketMonitor::stop() {
    if (recording) stopRecording();
    esp_wifi_set_promiscuous(false);
    running = false;
    Serial.println("Packet monitor stopped");
}

void PacketMonitor::showStatus() {
    if (rssiSamples > 0) averageRssi = rssiSum / (int)rssiSamples;
    Serial.printf("Packet monitor: ch=%u packets=%lu mgmt=%lu data=%lu ctrl=%lu avgRssi=%d rec=%s\n",
                  currentChannel, packetCount, managementCount, dataCount, controlCount,
                  averageRssi, recording ? "on" : "off");
    Display::displayInfo("Packet Status",
                         "CH " + String(currentChannel) + " RSSI " + String(averageRssi),
                         "Packets " + String(packetCount),
                         recording ? "Recording ON" : "Recording OFF");
    delay(1600);
}

void PacketMonitor::showRawFiles() {
    if (!SD.begin(SD_CS)) {
        Serial.println("SD init failed");
        return;
    }
    File root = SD.open("/");
    Serial.println("\n=== Raw/PCAP files ===");
    while (true) {
        File entry = root.openNextFile();
        if (!entry) break;
        String name = entry.name();
        if (!entry.isDirectory() && (name.endsWith(".pcap") || name.endsWith(".raw"))) {
            Serial.printf("%s (%u bytes)\n", name.c_str(), (unsigned)entry.size());
        }
        entry.close();
    }
    root.close();
    Display::displayInfo("Raw Files", "Listed on serial", "Replay disabled", "Use PCAP viewer");
    delay(1800);
}

void PacketMonitor::flushBuffer() {
    if (monitorBuffer) monitorBuffer->forceSave(&SD);
    Serial.println("Packet buffer flushed");
    Display::displayInfo("Packet Buffer", "Flushed to SD", "", "");
    delay(1200);
}
