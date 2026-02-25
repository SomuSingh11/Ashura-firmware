#include "WiFiManager.h"
#include "EventBus.h"
#include "config.h"

void WiFiManager::init(const char*ssid, const char*password){
    _ssid = ssid;
    _password = password;
    WiFi.mode(WIFI_STA);
    WiFi.begin(_ssid, _password);
    Serial.println("\n📡 Connecting to WiFi...");

    // connection loop - wait upto 30 attemps
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 8000) {
        delay(200);
        Serial.print(".");
        yield();
    }
    Serial.println();

    if(WiFi.status() == WL_CONNECTED){
        Serial.println("\n✅ WiFi Connected! IP: " + WiFi.localIP().toString());
        _wasConnected = true;
        Bus().publish(AppEvent::WifiConnected, WiFi.localIP().toString());
    } else {
        Serial.println("\n❌ WiFi connection failed!");
        Bus().publish(AppEvent::WifiDisconnected);
    }
}

void WiFiManager::update() {
    bool now = isConnected();
    if(now && !_wasConnected){
        _wasConnected = true;
        Bus().publish(AppEvent::WifiConnected, WiFi.localIP().toString());
    } else if(!now && _wasConnected){
        _wasConnected = false;
        Bus().publish(AppEvent::WifiDisconnected);
    }

    // Auto Reconnect
    if(!now){
        unsigned long t = millis();
        if(t-_lastTry > RECONNECT_DELAY) {
            _lastTry = t;
            WiFi.reconnect();
        }
    }
}