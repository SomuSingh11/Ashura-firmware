#pragma once
#include <Arduino.h>

// ================================================================
//  WledDevice  —  A single discovered WLED device
//
//  Populated by WledDiscovery (mDNS) or loaded from NVS.
//  Passed around by value — small struct, safe to copy.
// ================================================================

struct WledDevice {
    String  name;      // Friendly device name from /json/info
    String  ip;        // Friendly device name from /json/info
    int     port = 80; // HTTP port (default = 80)

    bool isValid() const { return ip.length() > 0; }

    String baseUrl() const { return "http://" + ip + ":" + String(port); }

    String stateUrl() const { return baseUrl() + "/json/state"; } // URL to control LED state (on/off, brightness, effects)
    String infoUrl()  const { return baseUrl() + "/json/info"; }  // URL to fetch device information
    String jsonUrl()  const { return baseUrl() + "/json"; }       // General JSON API endpoint
};