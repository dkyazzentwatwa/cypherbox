// rfid_tools.h - MFRC522 RFID lab tools for Cypherbox V2

#ifndef RFID_TOOLS_H
#define RFID_TOOLS_H

#include <Arduino.h>
#include <MFRC522.h>
#include "../config.h"

class RfidTools {
public:
    static void init();
    static void runIdentify();
    static void runReadBlocks();
    static bool dumpToSd();
    static bool writeDumpToCard(const String& dumpPath);
    static void listDumps();
    static bool isReady() { return initialized; }

private:
    static bool waitForCard(unsigned long timeoutMs);
    static String uidString();
    static String cardTypeString();
    static uint16_t atqaValue();
    static uint16_t blockCount();
    static uint8_t trailerBlockFor(uint16_t block);
    static bool isTrailerBlock(uint16_t block);
    static bool authenticateBlock(uint16_t block);
    static bool readBlock(uint16_t block, byte* buffer);
    static bool writeBlock(uint16_t block, const byte* data);
    static void printBlock(uint16_t block, const byte* data);
    static String blockToHex(const byte* data);
    static bool hexToBlock(const String& hex, byte* data);
    static String nextDumpBasePath();
    static void haltCard();

    static MFRC522 reader;
    static MFRC522::MIFARE_Key defaultKey;
    static bool initialized;
};

#endif
