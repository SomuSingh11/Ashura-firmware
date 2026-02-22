#pragma once
#include<Arduino.h>
#include<WiFi.h>

class WiFiManager {
    public:
        void init(const char* ssid, const char*password);
        bool isConnected() const;
        String localIP() const;

    private:
        const char* _ssid;
        const char* _password;
};
