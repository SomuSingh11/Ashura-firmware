#pragma once
#include<Arduino.h>
#include<WiFi.h>
#include "config.h"

// ================================================================
//  WiFiManager  —  State machine WiFi with exponential backoff
//
//  States:
//    IDLE        no credentials saved, never tried
//    CONNECTING  actively attempting, backoff running
//    CONNECTED   WiFi up and running
//    LOST        was connected, dropped, reconnecting
//    FAILED      gave up after MAX_ATTEMPTS, needs manual retry
//
//  Backoff sequence (ms): 2000 4000 8000 16000 32000 60000 60000 60000
//  MAX_ATTEMPTS: 8  →  after that, state = FAILED, stops retrying
//
//  Boot behavior:
//    init() loads credentials from NVS.
//    If no SSID saved → stays IDLE (no spam, no false failures).
//    If SSID saved → starts CONNECTING immediately.
//
//  NVS keys (namespace "network"):
//    net_ssid   → WiFi SSID
//    net_pass   → WiFi password
// ================================================================

enum class NetState {
    IDLE,
    CONNECTING,
    CONNECTED, 
    LOST,
    FAILED 
};

class WiFiManager {
    public:
        // ── Lifecycle ─────────────────────────────────────────────
        void init();        // loads NVS creds, starts connecting if available
        void update();      // call every loop()

        // ── Manual actions ────────────────────────────────────────
        void manualRetry();         // Settings -> Retry
        void forget();              // Clear NVS -> go IDLE
        void saveCredentials(const String& ssid, const String& password); // Save + Trigger connect

        // ── State ─────────────────────────────────────────────────
        NetState    state()          const { return _state; }
        bool        isConnected()    const {return _state == NetState::CONNECTED; }
        int         attemptCount()   const { return _attempts; }
        String      localIp()        const { return WiFi.localIP().toString(); }
        String      ssid()           const { return _ssid; }
        int         rssi()           const { return WiFi.RSSI(); }
        bool        hasCredentials() const { return _ssid.length() > 0; }

        // ── NVS ───────────────────────────────────────────────────
        void loadCredentials();

    private:
        void _startConnecting();
        void _onConnected();
        void _onFailed();
        unsigned long _nextBackoff();

        String          _ssid;
        String          _password;
        NetState        _state              = NetState::IDLE;
        int             _attempts           = 0;
        unsigned long   _retryAfter         = 0;        // millis() when next attempt is allowed
        unsigned long   _attemptStart       = 0;        // millis() when current WiFi.begin() fired
        bool            _attemptInFlight    = false;        
};
