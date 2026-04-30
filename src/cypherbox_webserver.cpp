// webserver.cpp - Web Server with Captive Portal Implementation for Cypherbox V2

#include "cypherbox_webserver.h"
#include "wifi_scanner.h"
#include "ble_scanner.h"
#include "../config.h"

WebServer* StarbeamWebServer::server = nullptr;
DNSServer* StarbeamWebServer::dnsServer = nullptr;
bool StarbeamWebServer::running = false;
IPAddress StarbeamWebServer::apIP(192, 168, 4, 1);
bool StarbeamWebServer::wifiScanEnabled = false;
bool StarbeamWebServer::bleScanEnabled = false;

void StarbeamWebServer::init() {
    server = nullptr;
    dnsServer = nullptr;
    running = false;
}

void StarbeamWebServer::start() {
    if (running) return;
    Serial.println("Starting web server...");

    WiFi.mode(WIFI_AP_STA);
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
    WiFi.softAP(AP_SSID, AP_PASSWORD);
    delay(500);

    dnsServer = new DNSServer();
    dnsServer->start(53, "*", apIP);

    server = new WebServer(WEB_SERVER_PORT);

    server->on("/", handleRoot);
    server->on("/wifi", handleWiFi);
    server->on("/ble", handleBLE);
    server->on("/api/wifi", handleWiFiJSON);
    server->on("/api/ble", handleBLEJSON);
    server->on("/api/scan/control", HTTP_POST, handleScanControl);
    server->on("/api/scan/status", HTTP_GET, handleStatus);
    server->on("/security", handleSecurityRoot);
    server->onNotFound(handleNotFound);

    server->begin();
    running = true;
    Serial.printf("Web server + captive portal started at http://%s\n", apIP.toString().c_str());
    Display::displayInfo("WEB SERVER",
                         "SSID: " + String(AP_SSID),
                         "IP: " + apIP.toString(),
                         "Captive Portal: ON");
}

void StarbeamWebServer::stop() {
    if (!running) return;
    server->stop();
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_OFF);
    delete dnsServer;
    delete server;
    dnsServer = nullptr;
    server = nullptr;
    running = false;
    Serial.println("Web server stopped");
}

void StarbeamWebServer::handleClient() {
    if (running && server) {
        dnsServer->processNextRequest();
        server->handleClient();
    }
}

bool StarbeamWebServer::isRunning() { return running; }
String StarbeamWebServer::getIP() { return apIP.toString(); }

String StarbeamWebServer::generateHTMLHeader(const char* title) {
    String html = "<!DOCTYPE html><html><head><meta charset='utf-8'>";
    html += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
    html += "<title>Cypherbox - " + String(title) + "</title>";
    html += "<style>";
    html += "*{box-sizing:border-box;margin:0;padding:0}";
    html += "body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,sans-serif;background:#1a1a2e;color:#eee;min-height:100vh}";
    html += "nav{background:#16213e;padding:1rem;display:flex;flex-wrap:wrap;gap:.5rem}";
    html += "nav a{padding:.5rem 1rem;background:#0f3460;color:#fff;text-decoration:none;border-radius:4px;font-size:.9rem}";
    html += "nav a:hover{background:#e94560}";
    html += ".container{max-width:900px;margin:0 auto;padding:1rem}";
    html += "h1{color:#e94560;margin-bottom:1rem;font-size:1.4rem}";
    html += ".card{background:#16213e;border-radius:8px;padding:1rem;margin-bottom:1rem;overflow-x:auto}";
    html += "table{width:100%;border-collapse:collapse;min-width:500px}";
    html += "th,td{padding:.6rem;text-align:left;border-bottom:1px solid #0f3460;font-size:.85rem}";
    html += "th{color:#e94560;background:#0f3460}";
    html += ".badge{padding:.2rem .5rem;border-radius:3px;font-size:.75rem}";
    html += ".badge-green{background:#10b981}.badge-red{background:#ef4444}";
    html += ".badge-yellow{background:#f59e0b}.badge-gray{background:#6b7280}";
    html += "form{display:inline}button{padding:.5rem 1rem;background:#e94560;color:#fff;border:none;border-radius:4px;cursor:pointer}";
    html += "button:hover{opacity:.9}";
    html += ".status-row{display:flex;gap:1rem;flex-wrap:wrap;margin-bottom:1rem}";
    html += ".status-item{background:#16213e;padding:1rem;border-radius:8px;flex:1;min-width:140px}";
    html += ".status-item h3{font-size:.8rem;color:#888;margin-bottom:.3rem}";
    html += ".status-item p{font-size:1.2rem;font-weight:bold;color:#e94560}";
    html += "footer{text-align:center;padding:1rem;color:#555;font-size:.8rem}";
    html += "@media(max-width:600px){table{font-size:.75rem}th,td{padding:.4rem}}";
    html += "</style></head><body>";
    html += generateNavigation();
    html += "<div class='container'>";
    html += "<h1>" + String(title) + "</h1>";
    return html;
}

String StarbeamWebServer::generateNavigation() {
    String nav = "<nav>";
    nav += "<a href='/'>Home</a>";
    nav += "<a href='/wifi'>WiFi</a>";
    nav += "<a href='/ble'>BLE</a>";
    nav += "<a href='/security'>Lab Modes</a>";
    nav += "</nav>";
    return nav;
}

String StarbeamWebServer::generateHTMLFooter() {
    return "<footer>Cypherbox V2 | ESP32 Cybersecurity Toolkit</footer></div></body></html>";
}

void StarbeamWebServer::handleRoot() {
    String html = generateHTMLHeader("Dashboard");
    html += "<div class='status-row'>";
    html += "<div class='status-item'><h3>WiFi Networks</h3><p>" + String(WiFiScanner::getNetworkCount()) + "</p></div>";
    html += "<div class='status-item'><h3>BLE Devices</h3><p>" + String(BLEScanner::getDeviceCount()) + "</p></div>";
    html += "<div class='status-item'><h3>Web Server</h3><p>" + String(running ? "ON" : "OFF") + "</p></div>";
    html += "<div class='status-item'><h3>Free Heap</h3><p>" + String(ESP.getFreeHeap() / 1024) + "KB</p></div>";
    html += "</div>";
    html += "<div class='card'><h2>Quick Controls</h2><br>";
    html += "<form action='/api/scan/control' method='POST'><input type='hidden' name='wifi' value='toggle'><button>Toggle WiFi Scan</button></form> ";
    html += "<form action='/api/scan/control' method='POST'><input type='hidden' name='ble' value='toggle'><button>Toggle BLE Scan</button></form> ";
    html += "</div>";
    html += generateHTMLFooter();
    server->send(200, "text/html", html);
}

void StarbeamWebServer::handleWiFi() {
    String html = generateHTMLHeader("WiFi Scanner");
    html += "<div class='card'><h2>WiFi Networks (" + String(WiFiScanner::getNetworkCount()) + ")</h2>";
    html += generateWiFiTable();
    html += "</div>";
    html += generateHTMLFooter();
    server->send(200, "text/html", html);
}

void StarbeamWebServer::handleBLE() {
    String html = generateHTMLHeader("BLE Scanner");
    html += "<div class='card'><h2>BLE Devices (" + String(BLEScanner::getDeviceCount()) + ")</h2>";
    html += generateBLETable();
    html += "</div>";
    html += generateHTMLFooter();
    server->send(200, "text/html", html);
}

String StarbeamWebServer::generateWiFiTable() {
    String table = "<table><tr><th>#</th><th>SSID</th><th>RSSI</th><th>Ch</th><th>Security</th></tr>";
    for (int i = 0; i < WiFiScanner::getNetworkCount(); i++) {
        const WiFiNetworkInfo* net = WiFiScanner::getNetwork(i);
        if (!net) continue;
        String sec = getSecurityString(net->authMode);
        String secClass = (sec == "OPEN") ? "badge-red" : "badge-green";
        int rssiBars = map(net->rssi, -100, -30, 0, 4);
        String rssiDisplay = String(net->rssi) + " dBm " + String(rssiBars >= 3 ? "#####" : rssiBars >= 2 ? "####" : rssiBars >= 1 ? "###" : "##");
        table += "<tr><td>" + String(i + 1) + "</td><td>" + (net->ssid.length() > 0 ? net->ssid : "(hidden)") + "</td><td>" + rssiDisplay + "</td><td>" + String(net->channel) + "</td><td><span class='badge " + secClass + "'>" + sec + "</span></td></tr>";
    }
    table += "</table>";
    return table;
}

String StarbeamWebServer::generateBLETable() {
    String table = "<table><tr><th>#</th><th>Name</th><th>Address</th><th>RSSI</th><th>Type</th></tr>";
    for (int i = 0; i < BLEScanner::getDeviceCount(); i++) {
        const BLEDeviceInfo* dev = BLEScanner::getDevice(i);
        if (!dev) continue;
        String typeClass = dev->connectable ? "badge-green" : "badge-gray";
        String typeStr = dev->connectable ? "Connectable" : "Broadcast";
        table += "<tr><td>" + String(i + 1) + "</td><td>" + String(dev->name) + "</td><td>" + dev->address + "</td><td>" + String(dev->rssi) + " dBm</td><td><span class='badge " + typeClass + "'>" + typeStr + "</span></td></tr>";
    }
    table += "</table>";
    return table;
}

String StarbeamWebServer::getSecurityString(uint8_t authMode) {
    switch (authMode) {
        case WIFI_AUTH_OPEN: return "OPEN";
        case WIFI_AUTH_WEP: return "WEP";
        case WIFI_AUTH_WPA_PSK: return "WPA";
        case WIFI_AUTH_WPA2_PSK: return "WPA2";
        case WIFI_AUTH_WPA_WPA2_PSK: return "WPA/WPA2";
        case WIFI_AUTH_WPA2_ENTERPRISE: return "WPA2-EAP";
        case WIFI_AUTH_WPA3_PSK: return "WPA3";
        case WIFI_AUTH_WPA2_WPA3_PSK: return "WPA2/3";
        default: return "UNKNOWN";
    }
}

void StarbeamWebServer::handleWiFiJSON() {
    String json = "{\"networks\":[";
    for (int i = 0; i < WiFiScanner::getNetworkCount(); i++) {
        const WiFiNetworkInfo* net = WiFiScanner::getNetwork(i);
        if (net) {
            if (i > 0) json += ",";
            json += "{\"ssid\":\"" + net->ssid + "\",\"rssi\":" + String(net->rssi) + ",\"channel\":" + String(net->channel) + ",\"authMode\":" + String(net->authMode) + "}";
        }
    }
    json += "]}";
    server->send(200, "application/json", json);
}

void StarbeamWebServer::handleBLEJSON() {
    String json = "{\"devices\":[";
    for (int i = 0; i < BLEScanner::getDeviceCount(); i++) {
        const BLEDeviceInfo* dev = BLEScanner::getDevice(i);
        if (dev) {
            if (i > 0) json += ",";
            json += "{\"name\":\"" + String(dev->name) + "\",\"address\":\"" + String(dev->address) + "\",\"rssi\":" + String(dev->rssi) + "}";
        }
    }
    json += "]}";
    server->send(200, "application/json", json);
}

void StarbeamWebServer::handleScanControl() {
    // Basic toggle logic placeholder
    server->send(200, "text/plain", "OK");
}

void StarbeamWebServer::handleStatus() {
    String json = "{\"running\":" + String(running ? "true" : "false") + ",\"ip\":\"" + apIP.toString() + "\",\"wifiScan\":" + String(wifiScanEnabled ? "true" : "false") + ",\"bleScan\":" + String(bleScanEnabled ? "true" : "false") + "}";
    server->send(200, "application/json", json);
}

void StarbeamWebServer::handleSecurityRoot() {
    String html = generateHTMLHeader("Lab Modes Removed");
    html += "<div class='card'><h2>Non-attack build</h2><p>Deauth, evil-twin, beacon, probe, and PMKID attack controls are removed from this Cypherbox V2 build. Use WiFi scan, heatmap, packet monitor, RFID lab tools, BLE scan, and SD utilities instead.</p></div>";
    html += generateHTMLFooter();
    server->send(200, "text/html", html);
}

void StarbeamWebServer::handleNotFound() {
    if (running) {
        server->sendHeader("Location", "http://" + apIP.toString());
        server->send(302, "text/plain", "");
    } else {
        server->send(404, "text/plain", "Not Found");
    }
}
