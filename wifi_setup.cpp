#include "wifi_setup.h"
#include "config.h"
#include "ui_draw.h"
#include "web_portal.h"
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>

WebServer server(80);
DNSServer dnsServer;
Preferences prefs;

String apName = ""; 
const byte DNS_PORT = 53;

bool triggerConnect = false;
bool isConnecting = false;
unsigned long connectStartTime = 0;
String customSSID = "";
String customPass = "";
bool isPortalRunning = false;
bool pendingStopPortal = false; 
unsigned long stopPortalTime = 0;
unsigned long lastReconnectAttempt = 0;

void startCaptivePortal() {
    if (isPortalRunning) return;
    
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(apName.c_str());
    dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());

    server.on("/", HTTP_GET, []() {
        server.send(200, "text/html", index_html);
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
            json += "{\"ssid\":\"" + WiFi.SSID(i) + "\",\"rssi\":" + String(WiFi.RSSI(i)) + "}";
        }
        json += "]";
        
        server.send(200, "application/json", json);

        WiFi.scanDelete(); 

        if (customSSID != "") {
            WiFi.begin(customSSID.c_str(), customPass.c_str());
            WiFi.setAutoReconnect(wasAuto);
        }
    });

    server.on("/save", HTTP_POST, []() {
        customSSID = server.arg("ssid");
        customPass = server.arg("pass");
        server.send(200, "text/plain", "OK");
        triggerConnect = true; 
    });

    server.on("/status", HTTP_GET, []() {
        if (isConnecting) {
            server.send(200, "text/plain", "connecting");
        } else if (WiFi.status() == WL_CONNECTED) {
            server.send(200, "text/plain", "success");
        } else {
            server.send(200, "text/plain", "failed");
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
    apName = "OrchidPal_" + String((uint16_t)(chipid >> 32), HEX);

    prefs.begin("wifi_creds", false);
    customSSID = prefs.getString("ssid", "");
    customPass = prefs.getString("pass", "");
    prefs.end();

    if (customSSID == "") {
        setFooterMsg("Setup Mode: Connect to WiFi '" + apName + "'");
        startCaptivePortal();
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
            updateHeader(true, false); 
        } else {
            Serial.println("Failed to connect to saved WiFi. Starting Portal...");
            forceFooterMsg("WiFi Failed! Config at '" + apName + "'");
            startCaptivePortal();
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
            
            prefs.begin("wifi_creds", false);
            prefs.putString("ssid", customSSID);
            prefs.putString("pass", customPass);
            prefs.end();
            
            updateHeader(true, false);
            
            pendingStopPortal = true;
            stopPortalTime = millis();
            
        } else if (millis() - connectStartTime > 6000) {
            isConnecting = false;
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

    if (!triggerConnect && !isConnecting && !pendingStopPortal) {
        static bool lastStatus = false;
        bool currentStatus = (WiFi.status() == WL_CONNECTED);

        if (!currentStatus && lastStatus) {
            Serial.println("WiFi Connection Lost!");
            startCaptivePortal(); 
            WiFi.setAutoReconnect(true); 
            WiFi.begin(customSSID.c_str(), customPass.c_str()); 
            
            forceFooterMsg("Lost Connect! Config at '" + apName + "'"); 
            updateHeader(false, false);
        }
        lastStatus = currentStatus;
    }
}

void resetWifiConfig() {
    prefs.begin("wifi_creds", false);
    prefs.clear(); 
    prefs.end();
    Serial.println("WiFi Settings Cleared!");
    ESP.restart();
}
