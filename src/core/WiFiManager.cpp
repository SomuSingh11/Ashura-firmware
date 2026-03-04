#include "WiFiManager.h"
#include "EventBus.h"
#include "config.h"
#include <Preferences.h>
#include "NotificationManager.h"

// ================================================================
//  WiFiManager.cpp
// ================================================================

// ── Lifecycle ─────────────────────────────────────────────────
void WiFiManager::init() {
    WiFi.mode(WIFI_STA);
    WiFi.setAutoConnect(false); // We manage reconnect ourselves

    loadCredentials();

    if(_ssid.length() > 0){
        Serial.println("[WiFi] Credentials found, connecting...");
        _startConnecting();
    } else {
        Serial.println("[WiFi] No credentials — staying IDLE");
        _state = NetState::IDLE;
    }
}

void WiFiManager::update() {
    switch(_state){
        case NetState::IDLE:
        case NetState::FAILED:
            break;     // Do nothing - waiting for manual retry
        
        case NetState::CONNECTED: {
            if(WiFi.status() != WL_CONNECTED){
                Serial.println("[WiFi] Connection lost");
                _state              = NetState::LOST;
                _attempts           = 0;
                _attemptInFlight    = false;
                _retryAfter         = 0;
                Bus().publish(AppEvent::WifiDisconnected);
            }
            break;
        }

        case NetState::CONNECTING:
        case NetState::LOST: {
            // 1. Check if WiFi came up
            bool up = WiFi.status() == WL_CONNECTED;
            if(up) {
                _onConnected();
                break;
            }

            unsigned long now = millis();

            // 2. Sitting out a backoff delay — do nothing
            if (now < _retryAfter) break;


            // 3. Attempt is in flight — check for timeout
            if (_attemptInFlight) {
                if (now - _attemptStart < Config::WiFi::CONNECT_TIMEOUT) break; // still waiting

                // Timed out
                _attemptInFlight = false;
                _attempts++;
                Serial.println("[WiFi] Attempt " + String(_attempts) +
                               "/" + String(Config::WiFi::MAX_ATTEMPTS) + " timed out");

                if (_attempts >= Config::WiFi::MAX_ATTEMPTS) {
                    _onFailed();
                } else {
                    unsigned long wait = _nextBackoff();
                    _retryAfter = now + wait;
                    Serial.println("[WiFi] Backoff " +
                                   String(wait / 1000) + "s");
                }
                break;
            }

            // 4. Nothing in flight, backoff elapsed — fire next attempt
            Serial.println("[WiFi] Attempt " + String(_attempts + 1) +
                           "/" + String(Config::WiFi::MAX_ATTEMPTS));
            WiFi.begin(_ssid.c_str(), _password.c_str());
            _attemptStart    = now;
            _attemptInFlight = true;
            break;
        }
    }
}

// ── Manual actions ────────────────────────────────────────────

void WiFiManager::manualRetry() {
    if(_ssid.length() == 0) {
        Serial.println("[WiFi] manualRetry — no credentials");
        return;
    }

    Serial.println("[WiFi] Manual retry triggered");
    _attempts        = 0;
    _attemptInFlight = false;
    _retryAfter      = 0;
    _startConnecting();
}

void WiFiManager::forget() {
    Serial.println("[WiFi] Forgetting network credentials");
    WiFi.disconnect(true);
    
    _ssid            = "";
    _password        = "";
    _state           = NetState::IDLE;
    _attempts        = 0;
    _attemptInFlight = false;

    Preferences p;
    p.begin("network", false);
    p.remove("net_ssid");
    p.remove("net_pass");
    p.end();

    Bus().publish(AppEvent::WifiIdle);
}

void WiFiManager::saveCredentials(const String& ssid, const String& pass) {
    _ssid       = ssid;
    _password   = pass;

    Preferences p;
    p.begin("network", false);
    p.putString("net_ssid", ssid);
    p.putString("net_pass", pass);
    p.end();

    Serial.println("[WiFi] Credentials saved for: " + ssid);

    // Immediately start connecting
    _attempts        = 0;
    _attemptInFlight = false;
    _retryAfter      = 0;
    _startConnecting(); 
}


// ── NVS ───────────────────────────────────────────────────────

void WiFiManager::loadCredentials(){
    Preferences p;
    p.begin("network", true);
    _ssid       = p.getString("net_ssid", "");
    _password   = p.getString("net_pass", "");
    p.end();

    if (_ssid.length() > 0) {
        Serial.println("[WiFi] Loaded SSID: " + _ssid);
    } else {
        Serial.println("[WiFi] No saved credentials");
    }
}


// ── Private ───────────────────────────────────────────────────

void WiFiManager::_startConnecting(){
    _state = NetState::CONNECTING;

    unsigned long now = millis();

    WiFi.begin(_ssid.c_str(), _password.c_str());

    _attemptStart     = now;
    _attemptInFlight  = true;
    _retryAfter       = 0;

    Serial.println("[WiFi] Connecting to: " + _ssid);
}

void WiFiManager::_onConnected() {
    _state          = NetState::CONNECTED;
    _attempts       = 0;
    _attemptInFlight= false;

    Serial.println("[WiFi] Connected! IP: " + localIp());
    Bus().publish(AppEvent::WifiConnected, localIp());
}

void WiFiManager::_onFailed() {
    _state   = NetState::FAILED;
    _attemptInFlight = false;
    WiFi.disconnect();
    Serial.println("[WiFi] Failed to connect after " + String(_attempts) + " attempts");
    
    NotifMgr().push(
        NotificationType::SYSTEM,
        "WiFi Unavailable",
        "Failed after " + String(Config::WiFi::MAX_ATTEMPTS) +
        " attempts. Go to Settings > Network to retry."
    );
    Bus().publish(AppEvent::WifiFailed);
}

unsigned long WiFiManager::_nextBackoff() {
    // 2s 4s 8s 16s 32s 60s 60s 60s...
    unsigned long backoff = Config::WiFi::BACKOFF_BASE;
    for (int i = 0; i < _attempts && backoff < Config::WiFi::BACKOFF_CAP; i++) {
        backoff *= 2;
    }
    return min(backoff, Config::WiFi::BACKOFF_CAP);
}