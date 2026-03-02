#pragma once

#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "config.h"
#include "../../application/wled/WledDevice.h"
#include "../../application/wled/WledState.h"


// ================================================================
//  WledClient  —  HTTP interface to one WLED device
//
//  All calls are blocking. Caller should show a loading indicator
//  before calling and hide it after.
//
//  Usage:
//    WledClient client;
//    client.setDevice(device);
//    WledState state;
//    if (client.fetchState(state)) { ... }
//    if (client.fetchEffects(effects)) { ... }
//    client.postJson(state.jsonPower());
// ================================================================

class WledClient {
    public:
        void setDevice(const WledDevice& dev){
            _device = dev;
        }

        const WledDevice& device() const { return _device; }

        bool hasDevice() const { return _device.isValid(); } 

        // ── Fetch current state ───────────────────────────────────
        // GET /json/state
        bool fetchState(WledState& out){
            String body;
            if(!_get(_device.stateUrl(),  body)) return false; // GET failed

            DynamicJsonDocument doc(1024);
            if(deserializeJson(doc, body) != DeserializationError::Ok) return false;

            out.on         = doc["on"]  | false;
            out.brightness = doc["bri"] | 128;

            JsonArray segs = doc["seg"];
            if (segs.size() > 0) {
                out.effectIndex = segs[0]["fx"]  | 0;
                out.speed     = segs[0]["sx"]  | 128;
                out.intensity = segs[0]["ix"]  | 128;
                JsonArray col = segs[0]["col"][0];
                if (col.size() >= 3) {
                    out.color.r = col[0];
                    out.color.g = col[1];
                    out.color.b = col[2];
                }
            }
            out.valid = true;
            Serial.println("[WLED] State fetched from " + _device.ip);
            return true;
        }

        // ── Fetch effects list ────────────────────────────────────
        bool fetchEffects(std::vector<String>& out) {
            String body;
            if (!_get(_device.jsonUrl(), body)) return false;

            DynamicJsonDocument doc(32768);   // effects list is large
            if (deserializeJson(doc, body) != DeserializationError::Ok) return false;

            out.clear();
            JsonArray effects = doc["effects"];
            for (const char* fx : effects) {
                out.push_back(String(fx));
            }

            Serial.println("[WLED] " + String(out.size()) + " effects fetched");
            return true;
        }

        // ── Fetch device info (name) ──────────────────────────────
        bool fetchInfo(String& outName) {
            String body;
            if (!_get(_device.infoUrl(), body)) return false;

            DynamicJsonDocument doc(1024);
            if (deserializeJson(doc, body) != DeserializationError::Ok) return false;

            outName = doc["name"] | _device.ip;
            return true;
        }

        // ── Send partial JSON ─────────────────────────────────────
        bool postJson(const String& json) {
            if (!_device.isValid()) return false;
            Serial.println("[WLED] POST " + json);
            return _post(_device.stateUrl(), json);
        }

        // ── Convenience senders ───────────────────────────────────
        bool setPower(bool on) {
            WledState s; 
            s.on = on;
            return postJson(s.jsonPower());
        }

        bool setBrightness(uint8_t bri) {
            WledState s; 
            s.brightness = bri;
            return postJson(s.jsonBrightness());
        }

        bool setEffect(int idx) {
            WledState s; 
            s.effectIndex = idx;
            return postJson(s.jsonEffect());
        }

        bool setSpeed(uint8_t sx, uint8_t ix) {
            WledState s; 
            s.speed = sx; 
            s.intensity = ix;
            return postJson(s.jsonSpeed());
        }

        bool setColor(WledColor c) {
            WledState s; 
            s.color = c;
            return postJson(s.jsonColor());
        }

        // Last error string for debug
        const String& lastError() const { return _lastError; }

    private:
        WledDevice  _device;
        String      _lastError;

        // Perform HTTP GET request and return response body if successful
        bool _get(const String& url, String& body){
            HTTPClient http;                            // Create temporary HTTP client
            http.begin(url);                            // Initialize connection to URL
            http.setTimeout(WLED_HTTPCLIENT_TIMEOUT);   // Set request timeout (3 seconds)

            int code = http.GET();                      // Send GET request

            if(code == 200){                            // Success (HTTP 200 OK)
                body = http.getString();                // Read response body
                http.end();                             // Close connection
                return true;                            // Indicate success
            }

            _lastError = "GET" + url + " -> " + String(code);
            Serial.println("[WLED] ERR " + _lastError);
            http.end();
            return false;                               // Indicate failure
        }

        // Perform HTTP POST request with JSON payload
        bool _post(const String& url, const String& json){
            HTTPClient http;
            http.begin(url);                                     
            http.setTimeout(WLED_HTTPCLIENT_TIMEOUT);
            http.addHeader("Content-Type", "application/json"); // Set JSON header

            int code = http.POST(json);                         // Send POST request with body
            http.end();                                         // Close connection

            if(code == 200 || code == 204){ // Success (HTTP 200 OK or 204 No Content)
                return true;
            };

            _lastError = "POST " + url + " -> " + String(code);
            Serial.println("[WLED] ERR " + _lastError);
            return false;                                       // Indicate failure
        }
};