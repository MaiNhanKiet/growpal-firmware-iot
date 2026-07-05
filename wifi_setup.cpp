#include "wifi_setup.h"
#include "config.h"
#include "ui_draw.h"
#include "web_portal.h"
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <ArduinoJson.h>

WebServer server(80);
DNSServer dnsServer;
Preferences prefs;

String apName = "";
const byte DNS_PORT = 53;

bool triggerConnect = false;
bool isConnecting = false;
bool wifiFlashSaved = false;
unsigned long connectStartTime = 0;

/** Trạng thái Save WiFi — tránh race /status trả failed quá sớm. */
enum WifiSaveState : uint8_t {
    WIFI_SAVE_IDLE = 0,
    WIFI_SAVE_CONNECTING,
    WIFI_SAVE_SUCCESS,
    WIFI_SAVE_FAILED,
    WIFI_SAVE_FLASH_ERR
};
static volatile WifiSaveState wifiSaveState = WIFI_SAVE_IDLE;
String customSSID = "";
String customPass = "";
bool isPortalRunning = false;
bool pendingStopPortal = false;
unsigned long stopPortalTime = 0;
unsigned long lastReconnectAttempt = 0;

// Runtime MQTT credentials (factory defaults until user saves MQTT tab)
char     mqttServer[MQTT_HOST_LEN] = MQTT_DEFAULT_SERVER;
uint16_t mqttPort = MQTT_PORT_DEFAULT;
char     mqttUser[MQTT_USER_LEN] = MQTT_DEFAULT_USER;
char     mqttPass[MQTT_PASS_LEN] = MQTT_DEFAULT_PASS;

static void applyFactoryMqttDefaults() {
    strncpy(mqttServer, MQTT_DEFAULT_SERVER, sizeof(mqttServer) - 1);
    mqttServer[sizeof(mqttServer) - 1] = '\0';
    mqttPort = MQTT_PORT_DEFAULT;
    strncpy(mqttUser, MQTT_DEFAULT_USER, sizeof(mqttUser) - 1);
    mqttUser[sizeof(mqttUser) - 1] = '\0';
    strncpy(mqttPass, MQTT_DEFAULT_PASS, sizeof(mqttPass) - 1);
    mqttPass[sizeof(mqttPass) - 1] = '\0';
}

static void safeCopyToBuf(const String& src, char* dest, size_t destLen) {
    if (dest == NULL || destLen == 0) return;
    size_t n = src.length();
    if (n >= destLen) n = destLen - 1;
    if (n > 0) {
        memcpy(dest, src.c_str(), n);
    }
    dest[n] = '\0';
}

static uint16_t parseMqttPort(const String& portStr) {
    long port = portStr.toInt();
    if (port < 1 || port > 65535) return MQTT_PORT_DEFAULT;
    return (uint16_t)port;
}

void loadMqttConfig() {
    Preferences mqttPrefs;
    if (!mqttPrefs.begin("mqtt_creds", true)) {
        Serial.println("WARN: mqtt_creds namespace open failed (read) — using defaults");
        applyFactoryMqttDefaults();
        return;
    }

    bool hasCustom = mqttPrefs.isKey("server");

    if (hasCustom) {
        if (mqttPrefs.getString("server", mqttServer, sizeof(mqttServer)) == 0) {
            strncpy(mqttServer, MQTT_DEFAULT_SERVER, sizeof(mqttServer) - 1);
            mqttServer[sizeof(mqttServer) - 1] = '\0';
        }
        mqttPort = (uint16_t)mqttPrefs.getUShort("port", MQTT_PORT_DEFAULT);
        if (mqttPort == 0) mqttPort = MQTT_PORT_DEFAULT;

        if (mqttPrefs.getString("user", mqttUser, sizeof(mqttUser)) == 0) {
            mqttUser[0] = '\0';
        }
        if (mqttPrefs.getString("pass", mqttPass, sizeof(mqttPass)) == 0) {
            mqttPass[0] = '\0';
        }
    } else {
        applyFactoryMqttDefaults();
    }

    mqttPrefs.end();
    Serial.printf(">> MQTT config loaded: %s:%u user=%s (%s)\n",
                  mqttServer[0] ? mqttServer : "(none)",
                  mqttPort,
                  mqttUser[0] ? mqttUser : "(anon)",
                  hasCustom ? "flash" : "factory default");
}

/** Ghi MQTT Flash và đọc lại để xác nhận. */
static bool saveMqttConfigToFlash() {
    Preferences mqttPrefs;
    if (!mqttPrefs.begin("mqtt_creds", false)) {
        Serial.println("ERROR: mqtt_creds write open failed");
        return false;
    }

    mqttPrefs.putString("server", mqttServer);
    mqttPrefs.putUShort("port", mqttPort);
    mqttPrefs.putString("user", mqttUser);
    mqttPrefs.putString("pass", mqttPass);
    mqttPrefs.end();

    // Verify read-back
    char checkServer[MQTT_HOST_LEN];
    char checkUser[MQTT_USER_LEN];
    char checkPass[MQTT_PASS_LEN];
    uint16_t checkPort = 0;

    if (!mqttPrefs.begin("mqtt_creds", true)) {
        Serial.println("ERROR: mqtt_creds verify open failed");
        return false;
    }
    if (mqttPrefs.getString("server", checkServer, sizeof(checkServer)) == 0) {
        checkServer[0] = '\0';
    }
    checkPort = mqttPrefs.getUShort("port", 0);
    if (mqttPrefs.getString("user", checkUser, sizeof(checkUser)) == 0) {
        checkUser[0] = '\0';
    }
    if (mqttPrefs.getString("pass", checkPass, sizeof(checkPass)) == 0) {
        checkPass[0] = '\0';
    }
    mqttPrefs.end();

    bool ok = (strcmp(checkServer, mqttServer) == 0 &&
               checkPort == mqttPort &&
               strcmp(checkUser, mqttUser) == 0 &&
               strcmp(checkPass, mqttPass) == 0);

    Serial.println(ok ? ">> MQTT credentials saved + verified" : "ERROR: MQTT flash verify failed");
    return ok;
}

/** Ghi WiFi Flash và đọc lại để xác nhận. */
static bool saveWifiConfigToFlash() {
    if (!prefs.begin("wifi_creds", false)) {
        Serial.println("ERROR: wifi_creds write open failed");
        return false;
    }
    prefs.putString("ssid", customSSID);
    prefs.putString("pass", customPass);
    prefs.end();

    if (!prefs.begin("wifi_creds", true)) {
        Serial.println("ERROR: wifi_creds verify open failed");
        return false;
    }
    String checkSsid = prefs.getString("ssid", "");
    String checkPass = prefs.getString("pass", "");
    prefs.end();

    bool ok = (checkSsid == customSSID && checkPass == customPass);
    Serial.println(ok ? ">> WiFi credentials saved + verified" : "ERROR: WiFi flash verify failed");
    return ok;
}

static void applyMqttFromRequest() {
    safeCopyToBuf(server.arg("mqtt_server"), mqttServer, sizeof(mqttServer));
    mqttPort = parseMqttPort(server.arg("mqtt_port"));
    safeCopyToBuf(server.arg("mqtt_user"), mqttUser, sizeof(mqttUser));
    safeCopyToBuf(server.arg("mqtt_pass"), mqttPass, sizeof(mqttPass));
}

static void startCaptivePortal();

void openConfigPortal(const char* reason) {
    if (isPortalRunning) return;

    Serial.printf(">> Opening config portal: %s\n", reason ? reason : "");
    startCaptivePortal();

    char footer[96];
    snprintf(footer, sizeof(footer), "%s Config at '%s'",
             reason ? reason : "Setup:",
             apName.c_str());
    forceFooterMsg(footer);
    updateHeader(WiFi.status() == WL_CONNECTED, false);
}

static void startCaptivePortal() {
    if (isPortalRunning) return;

    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(apName.c_str());
    dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());

    server.on("/", HTTP_GET, []() {
        server.send_P(200, "text/html", index_html);
    });

    server.on("/scan", HTTP_GET, []() {
        bool wasAuto = WiFi.getAutoReconnect();
        WiFi.setAutoReconnect(false);
        WiFi.disconnect();
        delay(100);

        int n = WiFi.scanNetworks(false, true);

        String json = "[";
        for (int i = 0; i < n; ++i) {
            if (i > 0) json += ",";
            // Escape quotes in SSID for valid JSON
            String ssid = WiFi.SSID(i);
            ssid.replace("\\", "\\\\");
            ssid.replace("\"", "\\\"");
            json += "{\"ssid\":\"" + ssid + "\",\"rssi\":" + String(WiFi.RSSI(i)) + "}";
        }
        json += "]";

        server.send(200, "application/json", json);

        WiFi.scanDelete();

        if (customSSID != "") {
            WiFi.begin(customSSID.c_str(), customPass.c_str());
            WiFi.setAutoReconnect(wasAuto);
        }
    });

    server.on("/config", HTTP_GET, []() {
        StaticJsonDocument<384> doc;
        doc["mqtt_server"] = mqttServer;
        doc["mqtt_port"]   = mqttPort;
        doc["mqtt_user"]   = mqttUser;
        doc["mqtt_pass"]   = mqttPass;
        doc["wifi_ssid"]   = customSSID;
        doc["wifi_ok"]     = (WiFi.status() == WL_CONNECTED);

        char out[384];
        size_t n = serializeJson(doc, out, sizeof(out));
        if (n == 0 || n >= sizeof(out)) {
            server.send(500, "application/json", "{\"error\":\"serialize\"}");
            return;
        }
        server.send(200, "application/json", out);
    });

    // WiFi tab only — không đụng MQTT
    server.on("/save", HTTP_POST, []() {
        customSSID = server.arg("ssid");
        customPass = server.arg("pass");
        wifiFlashSaved = false;
        wifiSaveState = WIFI_SAVE_CONNECTING;
        server.send(200, "text/plain", "OK");
        triggerConnect = true;
    });

    // MQTT tab only — ghi Flash + force reconnect
    server.on("/save-mqtt", HTTP_POST, []() {
        if (!server.hasArg("mqtt_server") || server.arg("mqtt_server").length() == 0) {
            server.send(400, "text/plain", "mqtt_server required");
            return;
        }
        applyMqttFromRequest();
        if (!saveMqttConfigToFlash()) {
            server.send(500, "text/plain", "flash_error");
            return;
        }
        mqttForceReconnect = true;
        server.send(200, "text/plain", "OK");
    });

    // connecting | success | failed | save_error
    server.on("/status", HTTP_GET, []() {
        switch (wifiSaveState) {
            case WIFI_SAVE_CONNECTING:
                server.send(200, "text/plain", "connecting");
                break;
            case WIFI_SAVE_SUCCESS:
                server.send(200, "text/plain", "success");
                break;
            case WIFI_SAVE_FLASH_ERR:
                server.send(200, "text/plain", "save_error");
                break;
            case WIFI_SAVE_FAILED:
                server.send(200, "text/plain", "failed");
                break;
            default:
                server.send(200, "text/plain", "idle");
                break;
        }
    });

    server.onNotFound([]() {
        server.sendHeader("Location", String("http://") + WiFi.softAPIP().toString(), true);
        server.send(302, "text/plain", "");
    });

    server.begin();
    isPortalRunning = true;
    Serial.println("Custom Captive Portal Started!");
}

void stopCaptivePortal() {
    if (!isPortalRunning) return;
    dnsServer.stop();
    server.stop();
    WiFi.softAPdisconnect(true);
    isPortalRunning = false;
}

void initWifi() {
    uint64_t chipid = ESP.getEfuseMac();
    apName = "GrowPal_" + String((uint16_t)(chipid >> 32), HEX);

    loadMqttConfig();

    prefs.begin("wifi_creds", false);
    customSSID = prefs.getString("ssid", "");
    customPass = prefs.getString("pass", "");
    prefs.end();

    if (customSSID == "") {
        openConfigPortal("Setup Mode:");
    } else {
        setFooterMsg("Connecting to saved WiFi...");
        WiFi.mode(WIFI_STA);
        WiFi.begin(customSSID.c_str(), customPass.c_str());

        unsigned long startAttempt = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 5000) {
            delay(100);
        }

        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("WiFi Connected!");
            wifiFlashSaved = true;
            updateHeader(true, false);
        } else {
            Serial.println("Failed to connect to saved WiFi. Starting Portal...");
            openConfigPortal("WiFi Failed!");
        }
    }
}

void loopWifi() {
    if (isPortalRunning) {
        dnsServer.processNextRequest();
        server.handleClient();
    }

    if (WiFi.status() != WL_CONNECTED) {
        if (WiFi.softAPgetStationNum() > 0) {
            WiFi.setAutoReconnect(false);
        } else {
            if (millis() - lastReconnectAttempt > 10000) {
                lastReconnectAttempt = millis();
                if (customSSID != "") {
                    WiFi.begin(customSSID.c_str(), customPass.c_str());
                }
            }
        }
    } else {
        WiFi.setAutoReconnect(true);
    }

    if (triggerConnect) {
        triggerConnect = false;
        isConnecting = true;
        wifiFlashSaved = false;
        wifiSaveState = WIFI_SAVE_CONNECTING;
        connectStartTime = millis();

        WiFi.mode(WIFI_AP_STA);

        Serial.println("Connecting to: " + customSSID);
        forceFooterMsg("Verifying pass for '" + customSSID + "'...");

        WiFi.disconnect();
        delay(100);
        WiFi.begin(customSSID.c_str(), customPass.c_str());
    }

    if (isConnecting) {
        if (WiFi.status() == WL_CONNECTED) {
            isConnecting = false;
            Serial.println("Connection SUCCESS!");

            wifiFlashSaved = saveWifiConfigToFlash();
            if (!wifiFlashSaved) {
                wifiSaveState = WIFI_SAVE_FLASH_ERR;
                forceFooterMsg("WiFi OK but Flash save failed!");
            } else {
                wifiSaveState = WIFI_SAVE_SUCCESS;
                updateHeader(true, false);
                pendingStopPortal = true;
                stopPortalTime = millis();
            }

        } else if (millis() - connectStartTime > 8000) {
            isConnecting = false;
            wifiFlashSaved = false;
            wifiSaveState = WIFI_SAVE_FAILED;
            WiFi.disconnect();
            Serial.println("Connection FAILED (Wrong Pass)!");
            forceFooterMsg("WRONG PASSWORD! Please retry.");
        }
    }

    if (pendingStopPortal && millis() - stopPortalTime > 2000) {
        pendingStopPortal = false;
        stopCaptivePortal();
        WiFi.mode(WIFI_STA);
        Serial.println("Portal Stopped. Running on STA only.");
    }

    // Mất WiFi → bật lại SoftAP để cấu hình
    if (!triggerConnect && !isConnecting && !pendingStopPortal) {
        static bool lastStatus = false;
        bool currentStatus = (WiFi.status() == WL_CONNECTED);

        if (!currentStatus && lastStatus) {
            Serial.println("WiFi Connection Lost!");
            openConfigPortal("Lost WiFi!");
            WiFi.setAutoReconnect(true);
            if (customSSID != "") {
                WiFi.begin(customSSID.c_str(), customPass.c_str());
            }
        }
        lastStatus = currentStatus;
    }
}

void resetWifiConfig() {
    prefs.begin("wifi_creds", false);
    prefs.clear();
    prefs.end();

    Preferences mqttPrefs;
    if (mqttPrefs.begin("mqtt_creds", false)) {
        mqttPrefs.clear();
        mqttPrefs.end();
    }

    applyFactoryMqttDefaults();
    wifiFlashSaved = false;

    Serial.println("WiFi + MQTT Settings Cleared!");
    ESP.restart();
}
