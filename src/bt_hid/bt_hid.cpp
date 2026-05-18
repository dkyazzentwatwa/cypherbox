// bt_hid.cpp - Glue between Cypherbox state machine and the vendored
// BT-HID subsystem (BLE-HID + DuckyScript + payload menu + web editor).

#include "bt_hid.h"
#include "bt_hid_config.h"
#include "hid_ble.h"
#include "payload_menu.h"
#include "bt_hid_web.h"
#include "../display.h"
#include "../../config.h"

#include <SD.h>

extern bool sdInitialized;

bool BtHid::inited = false;

namespace {

void seedDefaultPayloadIfMissing() {
    if (!sdInitialized) return;
    if (!SD.exists(BT_HID_PAYLOAD_DIR)) {
        SD.mkdir(BT_HID_PAYLOAD_DIR);
    }
    String folder = String(BT_HID_PAYLOAD_DIR) + "/" + BT_HID_DEFAULT_PAYLOAD_FOLDER;
    if (!SD.exists(folder.c_str())) SD.mkdir(folder.c_str());
    String path = folder + "/" + BT_HID_DEFAULT_PAYLOAD_NAME;
    if (SD.exists(path.c_str())) return;
    File f = SD.open(path.c_str(), FILE_WRITE);
    if (!f) return;
    f.print(
        "REM Default payload - opens a rickroll in the host browser\n"
        "DELAY 500\n"
        "GUI r\n"
        "DELAY 300\n"
        "STRING https://youtu.be/oHg5SJYRHA0\n"
        "ENTER\n");
    f.close();
}

}  // namespace

void BtHid::init() {
    if (inited) return;
    Serial.println("BtHid::init");
    seedDefaultPayloadIfMissing();
    hidx::init(BT_HID_DEVICE_NAME, BT_HID_DEVICE_MANUF, BT_HID_DEVICE_BATTERY);
    bt_hid_web::init();
    payload_menu::init();
    inited = true;
}

void BtHid::runPayloadMenu() {
    if (!inited) init();
    Display::displayInfo("BT HID",
                         hidx::isConnected() ? "Host connected" : "Advertising...",
                         bt_hid_web::apSsid(),
                         "Long-UP exits");
    delay(800);
    payload_menu::run();
}

void BtHid::runByName(const char* path) {
    if (!inited) init();
    payload_menu::runByName(path);
}

void BtHid::reload() {
    if (inited) payload_menu::reload();
}

bool BtHid::isConnected() {
    return inited && hidx::isConnected();
}
