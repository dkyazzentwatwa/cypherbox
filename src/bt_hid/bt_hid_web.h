// bt_hid_web.h - Wi-Fi AP + HTTP payload editor for BT HID.

#pragma once

namespace bt_hid_web {

// Brings up the soft AP (cypherbox-XXXX / duckduck) and the HTTP server
// on port 80 with the payload editor UI + JSON API.
void init();

const char* apSsid();
const char* apIp();

}  // namespace bt_hid_web
