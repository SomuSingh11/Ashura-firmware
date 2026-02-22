#include "WiFiManager.h"
#include "EventBus.h"

void WiFiManager::init(const char*ssid, const char*password){
    _ssid = ssid;
    _password = password;

    Serial.println("\n📡 Connecting to WiFi...");
    Bus().publish(AppEvent::DisplayNeedsUpdate, "WiFi\nConnecting");

    WiFi.mode(WIFI_STA);
    WiFi.begin(_ssid, _password);

    // connection loop - wait upto 30 attemps
    int attempts = 0;
    while(WiFi.status() != WL_CONNECTED && attempts < 30){
        delay(50);
        Serial.print(".");
        attempts++;
    }

    if(WiFi.status() == WL_CONNECTED){
        Serial.println("\n✅ WiFi Connected! IP: " + WiFi.localIP().toString());
        Bus().publish(AppEvent::WifiConnected, WiFi.localIP().toString());
    } else {
        Serial.println("\n❌ WiFi connection failed!");
        Bus().publish(AppEvent::WifiDisconnected);
    }
}

bool WiFiManager::isConnected() const {
  return WiFi.status() == WL_CONNECTED;
}

String WiFiManager::localIP() const {
    return WiFi.localIP().toString();
}