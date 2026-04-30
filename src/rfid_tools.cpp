// rfid_tools.cpp - MFRC522 RFID lab tools for Cypherbox V2

#include "rfid_tools.h"
#include "display.h"
#include "input.h"
#include <SPI.h>
#include <SD.h>

MFRC522 RfidTools::reader(RFID_SS, RFID_RST);
MFRC522::MIFARE_Key RfidTools::defaultKey;
bool RfidTools::initialized = false;

void RfidTools::init() {
    if (initialized) return;
    SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
    pinMode(RFID_SS, OUTPUT);
    digitalWrite(RFID_SS, HIGH);
    reader.PCD_Init();
    for (byte i = 0; i < 6; i++) defaultKey.keyByte[i] = 0xFF;
    initialized = true;
    Serial.printf("MFRC522 ready, version 0x%02X\n", reader.PCD_ReadRegister(reader.VersionReg));
}

bool RfidTools::waitForCard(unsigned long timeoutMs) {
    init();
    unsigned long start = millis();
    while (timeoutMs == 0 || millis() - start < timeoutMs) {
        if (Input::isButtonPressed(BUTTON_SELECT)) return false;
        if (reader.PICC_IsNewCardPresent() && reader.PICC_ReadCardSerial()) return true;
        yield();
    }
    return false;
}

String RfidTools::uidString() {
    String uid = "";
    for (byte i = 0; i < reader.uid.size; i++) {
        if (uid.length() > 0) uid += ":";
        if (reader.uid.uidByte[i] < 0x10) uid += "0";
        uid += String(reader.uid.uidByte[i], HEX);
    }
    uid.toUpperCase();
    return uid;
}

String RfidTools::cardTypeString() {
    return String(reader.PICC_GetTypeName(reader.PICC_GetType(reader.uid.sak)));
}

uint16_t RfidTools::atqaValue() {
    byte bufferATQA[2] = {0};
    byte bufferSize = sizeof(bufferATQA);
    reader.PICC_WakeupA(bufferATQA, &bufferSize);
    return ((uint16_t)bufferATQA[0] << 8) | bufferATQA[1];
}

uint16_t RfidTools::blockCount() {
    MFRC522::PICC_Type type = reader.PICC_GetType(reader.uid.sak);
    switch (type) {
        case MFRC522::PICC_TYPE_MIFARE_MINI: return 20;
        case MFRC522::PICC_TYPE_MIFARE_1K: return 64;
        case MFRC522::PICC_TYPE_MIFARE_4K: return 256;
        default: return 0;
    }
}

uint8_t RfidTools::trailerBlockFor(uint16_t block) {
    if (block < 128) return (block / 4) * 4 + 3;
    return 128 + ((block - 128) / 16) * 16 + 15;
}

bool RfidTools::isTrailerBlock(uint16_t block) {
    return trailerBlockFor(block) == block;
}

bool RfidTools::authenticateBlock(uint16_t block) {
    MFRC522::StatusCode status = reader.PCD_Authenticate(
        MFRC522::PICC_CMD_MF_AUTH_KEY_A,
        trailerBlockFor(block),
        &defaultKey,
        &(reader.uid)
    );
    if (status != MFRC522::STATUS_OK) {
        Serial.printf("RFID auth failed block %u: %s\n", block, reader.GetStatusCodeName(status));
        return false;
    }
    return true;
}

bool RfidTools::readBlock(uint16_t block, byte* buffer) {
    if (!authenticateBlock(block)) return false;
    byte size = 18;
    MFRC522::StatusCode status = reader.MIFARE_Read(block, buffer, &size);
    if (status != MFRC522::STATUS_OK) {
        Serial.printf("RFID read failed block %u: %s\n", block, reader.GetStatusCodeName(status));
        return false;
    }
    return true;
}

bool RfidTools::writeBlock(uint16_t block, const byte* data) {
    if (block == 0 || isTrailerBlock(block)) {
        Serial.printf("RFID write skipped protected block %u\n", block);
        return false;
    }
    if (!authenticateBlock(block)) return false;
    MFRC522::StatusCode status = reader.MIFARE_Write(block, (byte*)data, 16);
    if (status != MFRC522::STATUS_OK) {
        Serial.printf("RFID write failed block %u: %s\n", block, reader.GetStatusCodeName(status));
        return false;
    }
    return true;
}

String RfidTools::blockToHex(const byte* data) {
    String out = "";
    for (byte i = 0; i < 16; i++) {
        if (data[i] < 0x10) out += "0";
        out += String(data[i], HEX);
    }
    out.toUpperCase();
    return out;
}

bool RfidTools::hexToBlock(const String& hex, byte* data) {
    if (hex.length() < 32) return false;
    for (byte i = 0; i < 16; i++) {
        char pair[3] = { hex.charAt(i * 2), hex.charAt(i * 2 + 1), '\0' };
        data[i] = strtoul(pair, nullptr, 16);
    }
    return true;
}

void RfidTools::printBlock(uint16_t block, const byte* data) {
    Serial.printf("Block %03u  %s  ", block, blockToHex(data).c_str());
    for (byte i = 0; i < 16; i++) {
        Serial.print((data[i] >= 32 && data[i] < 127) ? (char)data[i] : '.');
    }
    Serial.println();
}

String RfidTools::nextDumpBasePath() {
    if (!SD.exists("/rfid")) SD.mkdir("/rfid");
    String cleanUid = uidString();
    cleanUid.replace(":", "");
    String base = "/rfid/" + cleanUid + "-" + String(millis());
    return base;
}

void RfidTools::haltCard() {
    reader.PICC_HaltA();
    reader.PCD_StopCrypto1();
}

void RfidTools::runIdentify() {
    init();
    Display::displayInfo("RFID", "Scan a tag", "", "[SEL]=exit");
    delay(250);
    while (true) {
        if (!waitForCard(0)) return;
        uint16_t atqa = atqaValue();
        String uid = uidString();
        String type = cardTypeString();
        uint16_t blocks = blockCount();

        Serial.println("\n=== RFID Card ===");
        Serial.printf("UID: %s\n", uid.c_str());
        Serial.printf("Type: %s\n", type.c_str());
        Serial.printf("SAK: 0x%02X ATQA: 0x%04X UID bytes: %u Blocks: %u\n",
                      reader.uid.sak, atqa, reader.uid.size, blocks);

        Display::displayInfo("RFID Found",
                             "UID " + uid.substring(0, 17),
                             type.substring(0, 21),
                             "SAK " + String(reader.uid.sak, HEX) + " Blocks " + String(blocks));
        haltCard();
        unsigned long start = millis();
        while (millis() - start < 2500) {
            if (Input::isButtonPressed(BUTTON_SELECT)) return;
            yield();
        }
        Display::displayInfo("RFID", "Scan another tag", "", "[SEL]=exit");
    }
}

void RfidTools::runReadBlocks() {
    init();
    uint16_t selectedBlock = 0;
    Display::displayInfo("RFID Blocks", "Scan tag", "UP/DN block", "SEL exit");
    delay(250);
    while (true) {
        if (Input::isButtonPressed(BUTTON_SELECT)) return;
        if (Input::isButtonPressed(BUTTON_UP)) {
            selectedBlock = selectedBlock == 0 ? 0 : selectedBlock - 1;
            delay(180);
        }
        if (Input::isButtonPressed(BUTTON_DOWN)) {
            selectedBlock++;
            delay(180);
        }
        if (!reader.PICC_IsNewCardPresent() || !reader.PICC_ReadCardSerial()) {
            yield();
            continue;
        }

        uint16_t blocks = blockCount();
        if (blocks == 0) {
            Display::displayInfo("RFID Blocks", "Unsupported tag", cardTypeString(), "SEL exit");
            haltCard();
            continue;
        }
        if (selectedBlock >= blocks) selectedBlock = blocks - 1;

        byte buffer[18] = {0};
        if (readBlock(selectedBlock, buffer)) {
            String hex = blockToHex(buffer);
            printBlock(selectedBlock, buffer);
            Display::displayInfo("Block " + String(selectedBlock) + "/" + String(blocks - 1),
                                 hex.substring(0, 16),
                                 hex.substring(16, 32),
                                 "UP/DN nav SEL exit");
        } else {
            Display::displayInfo("Block " + String(selectedBlock),
                                 "Read/auth failed",
                                 "Default key only",
                                 "UP/DN nav SEL exit");
        }
        haltCard();
        delay(350);
    }
}

bool RfidTools::dumpToSd() {
    init();
    if (!SD.begin(SD_CS)) {
        Serial.println("RFID dump failed: SD init failed");
        Display::displayInfo("RFID Dump", "SD init failed", "", "");
        return false;
    }
    Display::displayInfo("RFID Dump", "Scan source tag", "Default key only", "[SEL]=cancel");
    delay(250);
    if (!waitForCard(0)) return false;

    uint16_t blocks = blockCount();
    if (blocks == 0) {
        Display::displayInfo("RFID Dump", "Unsupported tag", cardTypeString(), "");
        haltCard();
        return false;
    }

    String base = nextDumpBasePath();
    File dump = SD.open(base + ".dump", FILE_WRITE);
    File txt = SD.open(base + ".txt", FILE_WRITE);
    if (!dump || !txt) {
        Serial.println("RFID dump failed: could not create files");
        Display::displayInfo("RFID Dump", "File create failed", "", "");
        haltCard();
        return false;
    }

    String uid = uidString();
    String type = cardTypeString();
    dump.println("CYPHERBOX_RFID_DUMP_V1");
    dump.println("UID=" + uid);
    dump.println("TYPE=" + type);
    dump.println("BLOCKS=" + String(blocks));
    txt.println("Cypherbox RFID Dump");
    txt.println("UID: " + uid);
    txt.println("Type: " + type);
    txt.println("Writable restore skips block 0 and trailer blocks.");

    int readable = 0;
    for (uint16_t block = 0; block < blocks; block++) {
        byte data[18] = {0};
        if (readBlock(block, data)) {
            String hex = blockToHex(data);
            dump.println("BLOCK=" + String(block) + ":" + hex);
            txt.printf("Block %03u: %s\n", block, hex.c_str());
            readable++;
        } else {
            dump.println("BLOCK=" + String(block) + ":LOCKED");
            txt.printf("Block %03u: LOCKED\n", block);
        }
    }

    dump.close();
    txt.close();
    haltCard();
    Serial.printf("RFID dump saved: %s.dump (%d/%u blocks)\n", base.c_str(), readable, blocks);
    Display::displayInfo("RFID Dump Saved",
                         base.substring(0, 21),
                         String(readable) + "/" + String(blocks) + " blocks",
                         "");
    delay(2000);
    return true;
}

void RfidTools::listDumps() {
    if (!SD.begin(SD_CS)) {
        Serial.println("SD init failed");
        return;
    }
    File dir = SD.open("/rfid");
    if (!dir || !dir.isDirectory()) {
        Serial.println("No /rfid dumps found");
        return;
    }
    Serial.println("\n=== RFID Dumps ===");
    while (true) {
        File entry = dir.openNextFile();
        if (!entry) break;
        if (!entry.isDirectory()) Serial.println(entry.name());
        entry.close();
    }
    dir.close();
}

bool RfidTools::writeDumpToCard(const String& dumpPath) {
    init();
    if (!SD.begin(SD_CS)) {
        Serial.println("RFID write failed: SD init failed");
        return false;
    }
    String path = dumpPath;
    if (!path.startsWith("/")) path = "/rfid/" + path;
    if (!path.endsWith(".dump")) path += ".dump";

    File dump = SD.open(path, FILE_READ);
    if (!dump) {
        Serial.println("RFID write failed: dump not found " + path);
        Display::displayInfo("RFID Write", "Dump not found", path.substring(0, 21), "");
        return false;
    }

    Display::displayInfo("RFID Write", "Scan blank test", "card to restore", "SEL cancel");
    delay(250);
    if (!waitForCard(0)) {
        dump.close();
        return false;
    }

    int written = 0;
    int skipped = 0;
    while (dump.available()) {
        String line = dump.readStringUntil('\n');
        line.trim();
        if (!line.startsWith("BLOCK=")) continue;
        int colon = line.indexOf(':');
        if (colon < 0) continue;
        uint16_t block = line.substring(6, colon).toInt();
        String payload = line.substring(colon + 1);
        if (payload == "LOCKED" || block == 0 || isTrailerBlock(block)) {
            skipped++;
            continue;
        }
        byte data[16] = {0};
        if (hexToBlock(payload, data) && writeBlock(block, data)) written++;
        else skipped++;
        yield();
    }

    dump.close();
    haltCard();
    Serial.printf("RFID write complete: %d written, %d skipped\n", written, skipped);
    Display::displayInfo("RFID Write Done",
                         String(written) + " blocks written",
                         String(skipped) + " skipped",
                         "Block0/trailers safe");
    delay(2500);
    return written > 0;
}
