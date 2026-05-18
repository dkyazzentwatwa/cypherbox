// payload_menu.cpp - On-device payload browser for BT HID.
//
// Adapted from ESP32_BT_HID/menu.cpp but renders via cypherbox's Display
// class instead of the standalone HAL. Browser layout:
//   View::OsList  - pick folder (e.g. macos, windows, ios)
//   View::List    - pick *.duck file
//   View::Preview - show payload text, Select fires, Up scrolls
// Long-press UP at any view returns to the previous view (or exits at OsList).

#include "payload_menu.h"
#include "hid_ble.h"
#include "payload.h"
#include "bt_hid_config.h"
#include "../display.h"
#include "../input.h"
#include "../../config.h"

#include <Arduino.h>
#include <SD.h>
#include <algorithm>
#include <vector>
#include <string>

extern bool sdInitialized;

namespace {

enum class View { OsList, List, Preview, Done };

View view = View::OsList;
std::vector<std::string> folders;
std::vector<int> folderCounts;
std::vector<std::string> items;
std::string currentFolder;
int folderSel = 0, folderTop = 0;
int selected = 0, firstVisible = 0;
int scroll = 0;
std::string previewBody;
std::string currentName;

std::string prettifyLabel(const std::string& s) {
    std::string n = s;
    const std::string ext = ".duck";
    if (n.size() > ext.size() &&
        n.compare(n.size() - ext.size(), ext.size(), ext) == 0)
        n.erase(n.size() - ext.size());
    static const char* PREFIXES[] = {"macos-", "windows-", "ios-", "cross-"};
    for (auto p : PREFIXES) {
        size_t plen = strlen(p);
        if (n.compare(0, plen, p) == 0) {
            n.erase(0, plen);
            size_t dash = n.find('-');
            if (dash != std::string::npos) n.erase(0, dash + 1);
            break;
        }
    }
    if (n.size() > 20) n = n.substr(0, 19) + "~";
    return n;
}

String dirPath(const std::string& folder) {
    String p = BT_HID_PAYLOAD_DIR;
    if (!folder.empty()) { p += "/"; p += folder.c_str(); }
    return p;
}
String fullPath(const std::string& folder, const std::string& name) {
    String p = dirPath(folder);
    p += "/";
    p += name.c_str();
    return p;
}

void ensureDir(const std::string& folder) {
    if (!sdInitialized) return;
    if (!SD.exists(BT_HID_PAYLOAD_DIR)) {
        // Create parent chain.
        const char* slash = strrchr(BT_HID_PAYLOAD_DIR, '/');
        if (slash && slash != BT_HID_PAYLOAD_DIR) {
            String parent = String(BT_HID_PAYLOAD_DIR)
                .substring(0, slash - BT_HID_PAYLOAD_DIR);
            if (!SD.exists(parent.c_str())) SD.mkdir(parent.c_str());
        }
        SD.mkdir(BT_HID_PAYLOAD_DIR);
    }
    if (!folder.empty()) {
        String d = dirPath(folder);
        if (!SD.exists(d.c_str())) SD.mkdir(d.c_str());
    }
}

std::vector<std::string> listFolders() {
    std::vector<std::string> out;
    if (!sdInitialized) return out;
    ensureDir("");
    File root = SD.open(BT_HID_PAYLOAD_DIR);
    if (!root || !root.isDirectory()) return out;
    File f = root.openNextFile();
    while (f) {
        if (f.isDirectory()) {
            String n = f.name();
            int slash = n.lastIndexOf('/');
            if (slash >= 0) n = n.substring(slash + 1);
            if (n.length() > 0 && n != "." && n != "..")
                out.push_back(std::string(n.c_str()));
        }
        f = root.openNextFile();
    }
    return out;
}

std::vector<std::string> listPayloads(const std::string& folder) {
    std::vector<std::string> out;
    if (!sdInitialized) return out;
    ensureDir(folder);
    File dir = SD.open(dirPath(folder).c_str());
    if (!dir || !dir.isDirectory()) return out;
    File f = dir.openNextFile();
    while (f) {
        if (!f.isDirectory()) {
            String n = f.name();
            int slash = n.lastIndexOf('/');
            if (slash >= 0) n = n.substring(slash + 1);
            if (n.endsWith(".duck")) out.push_back(std::string(n.c_str()));
        }
        f = dir.openNextFile();
    }
    return out;
}

bool readPayload(const std::string& folder, const std::string& name,
                 std::string& out) {
    if (!sdInitialized) return false;
    File f = SD.open(fullPath(folder, name).c_str(), FILE_READ);
    if (!f) return false;
    out.clear();
    while (f.available()) out += (char)f.read();
    f.close();
    return true;
}

void rebuildFolders() {
    folders = listFolders();
    std::sort(folders.begin(), folders.end());
    folderCounts.clear();
    folderCounts.reserve(folders.size());
    for (auto& f : folders) folderCounts.push_back((int)listPayloads(f).size());
    if (folderSel >= (int)folders.size())
        folderSel = folders.empty() ? 0 : (int)folders.size() - 1;
    if (folderSel < 0) folderSel = 0;
    folderTop = 0;
}

void rebuildList() {
    items = listPayloads(currentFolder);
    std::sort(items.begin(), items.end());
    if (selected >= (int)items.size())
        selected = items.empty() ? 0 : (int)items.size() - 1;
    if (selected < 0) selected = 0;
    firstVisible = 0;
}

void drawMenuList(const char* title,
                  const std::vector<std::string>& labels,
                  int sel, int top) {
    Adafruit_SSD1306& oled = Display::getOled();
    oled.clearDisplay();
    oled.setTextColor(SSD1306_WHITE);
    oled.setTextSize(1);
    oled.setCursor(0, 0);
    oled.println(title);
    oled.drawFastHLine(0, 10, SCREEN_WIDTH, SSD1306_WHITE);
    const int rowH = 10;
    const int rows = (SCREEN_HEIGHT - 14) / rowH;
    for (int i = 0; i < rows; ++i) {
        int idx = top + i;
        if (idx < 0 || idx >= (int)labels.size()) break;
        int y = 14 + i * rowH;
        if (idx == sel) {
            oled.fillRect(0, y - 1, SCREEN_WIDTH, rowH, SSD1306_WHITE);
            oled.setTextColor(SSD1306_BLACK);
        } else {
            oled.setTextColor(SSD1306_WHITE);
        }
        oled.setCursor(2, y);
        oled.print(labels[idx].c_str());
    }
    oled.display();
}

void drawOsList() {
    if (folders.empty()) {
        items = listPayloads("");
        if (items.empty()) {
            Display::displayInfo("No payloads on SD",
                                 "Drop .duck files in",
                                 BT_HID_PAYLOAD_DIR, "[UP]=exit");
        } else {
            currentFolder = "";
            view = View::List;
            firstVisible = 0; selected = 0;
            std::vector<std::string> labels;
            labels.reserve(items.size());
            for (auto& it : items) labels.push_back(prettifyLabel(it));
            char t[40];
            snprintf(t, sizeof(t), "%s Payloads",
                     hidx::isConnected() ? "[BT]" : "[..]");
            drawMenuList(t, labels, selected, firstVisible);
        }
        return;
    }
    std::vector<std::string> labels;
    labels.reserve(folders.size());
    for (size_t i = 0; i < folders.size(); ++i) {
        char buf[40];
        int n = (i < folderCounts.size()) ? folderCounts[i] : 0;
        snprintf(buf, sizeof(buf), "%s (%d)", folders[i].c_str(), n);
        labels.push_back(buf);
    }
    char title[40];
    snprintf(title, sizeof(title), "%s BT-HID Folders",
             hidx::isConnected() ? "[BT]" : "[..]");
    drawMenuList(title, labels, folderSel, folderTop);
}

void drawList() {
    if (items.empty()) {
        Display::displayInfo("No payloads here",
                             currentFolder.c_str(), "", "[UP]=back");
        return;
    }
    std::vector<std::string> labels;
    labels.reserve(items.size());
    for (auto& it : items) labels.push_back(prettifyLabel(it));
    char title[40];
    snprintf(title, sizeof(title), "%s %s",
             hidx::isConnected() ? "[BT]" : "[..]",
             currentFolder.c_str());
    drawMenuList(title, labels, selected, firstVisible);
}

void drawPreview() {
    Adafruit_SSD1306& oled = Display::getOled();
    oled.clearDisplay();
    oled.setTextColor(SSD1306_WHITE);
    oled.setTextSize(1);
    oled.setCursor(0, 0);
    oled.print(prettifyLabel(currentName).c_str());
    oled.drawFastHLine(0, 10, SCREEN_WIDTH, SSD1306_WHITE);
    const int rowH = 9;
    const int rows = (SCREEN_HEIGHT - 14) / rowH;
    int line = 0, shown = 0;
    const char* p = previewBody.c_str();
    while (*p && shown < rows) {
        if (line >= scroll) {
            oled.setCursor(0, 14 + shown * rowH);
            while (*p && *p != '\n') { oled.print(*p); ++p; }
            ++shown;
        } else {
            while (*p && *p != '\n') ++p;
        }
        if (*p == '\n') ++p;
        ++line;
    }
    oled.display();
}

void enterFolder() {
    if (folders.empty()) return;
    currentFolder = folders[folderSel];
    rebuildList();
    view = View::List;
    drawList();
}

void enterPreview() {
    if (items.empty()) return;
    currentName = items[selected];
    previewBody.clear();
    readPayload(currentFolder, currentName, previewBody);
    scroll = 0;
    view = View::Preview;
    drawPreview();
}

void runCurrent() {
    if (!hidx::isConnected()) {
        Display::displayInfo("BLE not connected",
                             "Pair host first",
                             currentName.c_str(), "[SEL]=ack");
        view = View::Done;
        return;
    }
    Display::displayInfo("Firing payload:",
                         currentName.c_str(), "", "");
    payload::run(previewBody);
    Display::displayInfo("Payload complete",
                         currentName.c_str(), "", "[SEL]=back");
    view = View::Done;
}

// Edge-triggered button reader: returns true once per press transition,
// then suppresses until release. Keeps the modal loop responsive without
// the "double fire" problem from raw isButtonPressed().
struct Edge { bool held = false; };
Edge upE, dnE, selE;

bool edge(Edge& e, uint8_t pin) {
    bool now = Input::isButtonPressed(pin);
    bool out = false;
    if (now && !e.held) out = true;
    e.held = now;
    return out;
}

// Long-press UP (>=600 ms) = exit/back. Returns true once when threshold
// crossed during the current hold.
struct LongPress { uint32_t start = 0; bool fired = false; };
LongPress upLong;
bool longUp() {
    bool now = Input::isButtonPressed(BUTTON_UP);
    if (!now) { upLong.start = 0; upLong.fired = false; return false; }
    if (!upLong.start) upLong.start = millis();
    if (!upLong.fired && (millis() - upLong.start) >= 600) {
        upLong.fired = true;
        return true;
    }
    return false;
}

}  // namespace

namespace payload_menu {

void init() {
    rebuildFolders();
}

void reload() {
    rebuildFolders();
    if (view == View::OsList) drawOsList();
    else if (view == View::List) { rebuildList(); drawList(); }
}

void runByName(const char* path) {
    std::string raw = path ? path : "";
    std::string folder, base;
    size_t slash = raw.find('/');
    if (slash != std::string::npos) {
        folder = raw.substr(0, slash);
        base = raw.substr(slash + 1);
    } else {
        folder = currentFolder;
        base = raw;
    }
    currentFolder = folder;
    currentName = base;
    previewBody.clear();
    if (!readPayload(folder, base, previewBody)) return;
    payload::run(previewBody);
}

void run() {
    rebuildFolders();
    view = View::OsList;
    drawOsList();

    // Reset edge trackers so a Select that entered this menu doesn't fire here.
    upE = dnE = selE = {};
    upLong = {};

    while (true) {
        bool eUp  = edge(upE,  BUTTON_UP);
        bool eDn  = edge(dnE,  BUTTON_DOWN);
        bool eSel = edge(selE, BUTTON_SELECT);
        bool longExit = longUp();

        if (longExit) {
            // Always exit on long-press UP regardless of view.
            Display::displayInfo("BT HID", "Exiting...", "", "");
            delay(300);
            return;
        }

        switch (view) {
            case View::OsList: {
                if (eUp && !folders.empty()) {
                    folderSel = (folderSel - 1 + folders.size()) % folders.size();
                    if (folderSel < folderTop) folderTop = folderSel;
                    if (folderSel > folderTop + 3) folderTop = folderSel - 3;
                    if (folderTop < 0) folderTop = 0;
                    drawOsList();
                } else if (eDn && !folders.empty()) {
                    folderSel = (folderSel + 1) % folders.size();
                    if (folderSel < folderTop) folderTop = folderSel;
                    if (folderSel > folderTop + 3) folderTop = folderSel - 3;
                    if (folderTop < 0) folderTop = 0;
                    drawOsList();
                } else if (eSel && !folders.empty()) {
                    enterFolder();
                }
                break;
            }
            case View::List: {
                if (eUp && !items.empty()) {
                    selected = (selected - 1 + items.size()) % items.size();
                    if (selected < firstVisible) firstVisible = selected;
                    if (selected > firstVisible + 3) firstVisible = selected - 3;
                    if (firstVisible < 0) firstVisible = 0;
                    drawList();
                } else if (eDn && !items.empty()) {
                    selected = (selected + 1) % items.size();
                    if (selected < firstVisible) firstVisible = selected;
                    if (selected > firstVisible + 3) firstVisible = selected - 3;
                    if (firstVisible < 0) firstVisible = 0;
                    drawList();
                } else if (eSel && !items.empty()) {
                    enterPreview();
                }
                break;
            }
            case View::Preview: {
                if (eUp) {
                    if (scroll > 0) { --scroll; drawPreview(); }
                } else if (eDn) {
                    ++scroll; drawPreview();
                } else if (eSel) {
                    runCurrent();
                }
                break;
            }
            case View::Done: {
                if (eSel || eUp || eDn) {
                    view = View::List;
                    drawList();
                }
                break;
            }
        }

        delay(20);
        yield();
    }
}

}  // namespace payload_menu
