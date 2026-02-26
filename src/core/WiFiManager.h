#pragma once
#include<Arduino.h>
#include<WiFi.h>


class WiFiManager {
    public:
        void init(const char* ssid, const char*password);
        void update();
        bool isConnected() const { return WiFi.status() == WL_CONNECTED; }
        String localIP() const { return WiFi.localIP().toString(); }

    private:
        const char*     _ssid           = nullptr;
        const char*     _password       = nullptr;
        unsigned long   _lastTry        = 0;
        bool            _wasConnected   = false;
};
