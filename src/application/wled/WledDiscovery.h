#pragma once

#include <Arduino.h>
#include <ESPmDNS.h>
#include "../../application/wled/WledDevice.h"
#include "../../application/wled/WledClient.h"

// ================================================================
//  WledDiscovery  —  mDNS scan for WLED devices
//
//  WLED announces itself as _wled._tcp on the local network.
//  scan() blocks for up to timeoutMs (default 3000ms).
//
//  Usage:
//    std::vector<WledDevice> found;
//    WledDiscovery::scan(found, client);
//    // found now contains all discovered devices with names
// ================================================================

class WledDiscovery {
    public:
    // Scan for WLED devices via mDNS
    // Fills `out` with discovered devices (names resolved via /json/info)
    // Returns number of devices found
    static int scan(std::vector<WledDevice>& out, WledClient& client) {
        // Takes a vector to store found devices
        // Uses your WledClient to resolve names

        out.clear();                    // Start fresh every scan.
        if(!MDNS.begin("ashura")) {     // registers your ESP32 on the network with name: "ashura.local"
            Serial.println("[Discovery] mDNS init failed");
            return 0;
        }

        Serial.println("[Discovery] Scanning for WLED devices...");
        int found = MDNS.queryService("wled", "tcp");               // MDNS.queryService(const char* service, const char* proto);
        Serial.println("[Discovery] Found " + String(found) + " device(s)");

        for(int i=0; i<found; i++){
            WledDevice dev;
            dev.ip = MDNS.IP(i).toString();
            dev.port = MDNS.port(i);
            dev.name = MDNS.hostname(i);   // fallback to hostname

            // Try to get friendly name from WLED info endpoint
            client.setDevice(dev);
            String name;
            if(client.fetchInfo(name) && name.length() > 0){
                dev.name = name;
            }

            out.push_back(dev);
            Serial.println("[Discovery] + " + dev.name + " @ " + dev.ip);
        }
        return found;
    }
};