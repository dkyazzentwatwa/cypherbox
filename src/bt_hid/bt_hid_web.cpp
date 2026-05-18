// bt_hid_web.cpp - Wi-Fi AP + HTTP payload editor for BT HID.
// Adapted from ESP32_BT_HID/web.cpp. Uses ESPAsyncWebServer.

#include "bt_hid_web.h"
#include "bt_hid_config.h"
#include "hid_ble.h"
#include "payload_menu.h"
#include "../../config.h"

#include <Arduino.h>
#include <WiFi.h>
#include <SD.h>
#include <ESPAsyncWebServer.h>
#include <string>
#include <vector>

extern bool sdInitialized;

namespace {

AsyncWebServer server(BT_HID_HTTP_PORT);
char ssidBuf[32] = {0};
char ipBuf[20] = {0};

const char INDEX_HTML[] PROGMEM = R"HTML(
<!doctype html><html><head><meta charset=utf-8>
<meta name=viewport content="width=device-width,initial-scale=1">
<title>cypherbox HID</title>
<style>
body{font:14px system-ui,sans-serif;margin:0;background:#0e0e10;color:#eee}
header{padding:10px 14px;background:#1a1a1f;border-bottom:1px solid #333}
header b{color:#7cc4ff}
main{padding:14px;max-width:760px;margin:0 auto}
.row{display:flex;gap:8px;flex-wrap:wrap;margin-bottom:8px}
button,input,textarea,select{background:#1a1a1f;color:#eee;border:1px solid #333;border-radius:6px;padding:6px 10px;font:inherit}
button{cursor:pointer}button:hover{background:#252531}
.danger{border-color:#a33}
textarea{width:100%;height:360px;font-family:ui-monospace,monospace}
.muted{color:#888;font-size:12px}
.status{display:flex;gap:14px;margin:6px 0 14px}
.dot{display:inline-block;width:8px;height:8px;border-radius:50%;background:#666;margin-right:6px}
.dot.on{background:#3c3}
</style></head><body>
<header><b>cypherbox</b> &mdash; BLE HID payload deck</header>
<main>
<div class=status>
  <div><span id=dot class=dot></span><span id=stat>...</span></div>
  <div class=muted id=meta></div>
</div>
<div class=row>
  <select id=sel></select>
  <button onclick=load()>Open</button>
  <button onclick=run()>Run</button>
  <button class=danger onclick=del()>Delete</button>
  <button onclick=newp()>New</button>
  <button onclick=reload()>Reload SD</button>
</div>
<div class=row>
  <input id=name placeholder="folder/filename.duck" style=flex:1>
  <button onclick=save()>Save</button>
</div>
<textarea id=body placeholder="REM type DuckyScript here&#10;DELAY 500&#10;STRING hello world&#10;ENTER"></textarea>
<p class=muted>Payloads live on SD under /cypherbox/payloads/&lt;folder&gt;.
Supported: REM, STRING, STRINGLN, DELAY, ENTER, TAB, GUI/CTRL/ALT/SHIFT [key], F1-F12, arrows, REPEAT n.</p>
</main>
<script>
const $=id=>document.getElementById(id);
async function refresh(){
  const s=await(await fetch('/status')).json();
  $('stat').textContent=s.bt_connected?'BLE host connected':'BLE waiting for host';
  $('dot').className='dot'+(s.bt_connected?' on':'');
  $('meta').textContent=`SSID ${s.ssid}  |  ${s.payload_dir}  |  ${s.payloads.length} payloads`;
  const sel=$('sel');sel.innerHTML='';
  for(const n of s.payloads){const o=document.createElement('option');o.value=n;o.textContent=n;sel.appendChild(o)}
}
async function load(){const n=$('sel').value;if(!n)return;
  const t=await(await fetch('/payload/'+encodeURIComponent(n))).text();
  $('name').value=n;$('body').value=t;}
async function save(){const n=$('name').value.trim();if(!n)return alert('name?');
  await fetch('/payload/'+encodeURIComponent(n),{method:'POST',body:$('body').value});
  refresh();}
async function run(){const n=$('name').value.trim()||$('sel').value;if(!n)return;
  await fetch('/run/'+encodeURIComponent(n),{method:'POST'});}
async function del(){const n=$('sel').value;if(!n||!confirm('Delete '+n+'?'))return;
  await fetch('/payload/'+encodeURIComponent(n),{method:'DELETE'});refresh();}
async function reload(){await fetch('/reload',{method:'POST'});refresh();}
function newp(){$('name').value='macos/new.duck';$('body').value='REM new payload\nDELAY 500\nSTRING hello\nENTER\n';}
refresh();setInterval(refresh,3000);
</script>
</body></html>
)HTML";

void buildSsid() {
    uint8_t mac[6];
    WiFi.softAPmacAddress(mac);
    snprintf(ssidBuf, sizeof(ssidBuf), "%s%02X%02X",
             BT_HID_AP_SSID_PREFIX, mac[4], mac[5]);
}

void splitFolderName(const std::string& path,
                     std::string& folder, std::string& name) {
    size_t slash = path.find('/');
    if (slash == std::string::npos) {
        folder.clear();
        name = path;
    } else {
        folder = path.substr(0, slash);
        name = path.substr(slash + 1);
    }
}

String dirPath(const std::string& folder) {
    String p = BT_HID_PAYLOAD_DIR;
    if (!folder.empty()) { p += "/"; p += folder.c_str(); }
    return p;
}
String fullPath(const std::string& folder, const std::string& name) {
    return dirPath(folder) + "/" + name.c_str();
}

void ensureDir(const std::string& folder) {
    if (!sdInitialized) return;
    if (!SD.exists(BT_HID_PAYLOAD_DIR)) {
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

bool readFile(const std::string& folder, const std::string& name,
              std::string& out) {
    if (!sdInitialized) return false;
    File f = SD.open(fullPath(folder, name).c_str(), FILE_READ);
    if (!f) return false;
    out.clear();
    while (f.available()) out += (char)f.read();
    f.close();
    return true;
}

bool writeFile(const std::string& folder, const std::string& name,
               const std::string& body) {
    if (!sdInitialized) return false;
    ensureDir(folder);
    File f = SD.open(fullPath(folder, name).c_str(), FILE_WRITE);
    if (!f) return false;
    f.print(body.c_str());
    f.close();
    return true;
}

bool deleteFile(const std::string& folder, const std::string& name) {
    if (!sdInitialized) return false;
    return SD.remove(fullPath(folder, name).c_str());
}

String jsonStatus() {
    String s = "{";
    s += "\"ssid\":\"";    s += ssidBuf;        s += "\",";
    s += "\"ip\":\"";      s += ipBuf;          s += "\",";
    s += "\"board\":\"cypherbox\",";
    s += "\"payload_dir\":\""; s += BT_HID_PAYLOAD_DIR; s += "\",";
    s += "\"bt_connected\":"; s += hidx::isConnected() ? "true" : "false";
    s += ",\"payloads\":[";
    auto folders = listFolders();
    bool first = true;
    for (auto& folder : folders) {
        auto items = listPayloads(folder);
        for (auto& it : items) {
            if (!first) s += ","; first = false;
            s += "\"";
            s += folder.c_str(); s += "/"; s += it.c_str();
            s += "\"";
        }
    }
    auto rootItems = listPayloads("");
    for (auto& it : rootItems) {
        if (!first) s += ","; first = false;
        s += "\""; s += it.c_str(); s += "\"";
    }
    s += "]}";
    return s;
}

}  // namespace

namespace bt_hid_web {

void init() {
    WiFi.mode(WIFI_AP);
    WiFi.softAP("placeholder");
    buildSsid();
    WiFi.softAPdisconnect(true);
    WiFi.softAP(ssidBuf, BT_HID_AP_PASSWORD);
    IPAddress ip = WiFi.softAPIP();
    snprintf(ipBuf, sizeof(ipBuf), "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);

    server.on("/", HTTP_GET, [](AsyncWebServerRequest* req){
        AsyncWebServerResponse* r = req->beginResponse_P(200, "text/html", INDEX_HTML);
        req->send(r);
    });

    server.on("/status", HTTP_GET, [](AsyncWebServerRequest* req){
        req->send(200, "application/json", jsonStatus());
    });

    server.on("/reload", HTTP_POST, [](AsyncWebServerRequest* req){
        payload_menu::reload();
        req->send(200, "text/plain", "ok");
    });

    server.onNotFound([](AsyncWebServerRequest* req){
        String url = req->url();
        if (url.startsWith("/payload/")) {
            std::string path(url.c_str() + strlen("/payload/"));
            std::string folder, name;
            splitFolderName(path, folder, name);
            if (req->method() == HTTP_GET) {
                std::string body;
                if (name.empty() || !readFile(folder, name, body)) {
                    req->send(404, "text/plain", "not found");
                    return;
                }
                req->send(200, "text/plain", body.c_str());
                return;
            }
            if (req->method() == HTTP_DELETE) {
                bool ok = !name.empty() && deleteFile(folder, name);
                payload_menu::reload();
                req->send(ok ? 200 : 500, "text/plain", ok ? "ok" : "fail");
                return;
            }
            if (req->method() == HTTP_POST) {
                // handled via onRequestBody
                return;
            }
        }
        if (url.startsWith("/run/") && req->method() == HTTP_POST) {
            std::string path(url.c_str() + strlen("/run/"));
            if (path.empty()) { req->send(400, "text/plain", "no name"); return; }
            payload_menu::runByName(path.c_str());
            req->send(200, "text/plain", "fired");
            return;
        }
        req->send(404, "text/plain", "not found");
    });

    server.onRequestBody([](AsyncWebServerRequest* req, uint8_t* data, size_t len,
                            size_t index, size_t total){
        static std::string buf;
        if (req->method() != HTTP_POST) return;
        String url = req->url();
        if (!url.startsWith("/payload/")) return;
        if (index == 0) buf.clear();
        buf.append((const char*)data, len);
        if (index + len == total) {
            std::string path(url.c_str() + strlen("/payload/"));
            std::string folder, name;
            splitFolderName(path, folder, name);
            bool ok = !name.empty() && writeFile(folder, name, buf);
            buf.clear();
            payload_menu::reload();
            req->send(ok ? 200 : 500, "text/plain", ok ? "ok" : "fail");
        }
    });

    server.begin();
    Serial.printf("BT-HID HTTP up @ %s (SSID %s)\n", ipBuf, ssidBuf);
}

const char* apSsid() { return ssidBuf; }
const char* apIp()   { return ipBuf; }

}  // namespace bt_hid_web
