#pragma once
#include<Arduino.h>
#include<ArduinoWebsockets.h>
#include "config.h"

using namespace websockets;

// ================================================================
//  WebSocketManager  —  State machine WS with exponential backoff
//
//  States:
//    IDLE          waiting for WiFi, not trying
//    CONNECTING    attempt in flight, backoff running
//    CONNECTED     socket open, registration pending
//    REGISTERED    fully operational, heartbeat running
//    FAILED        gave up after MAX_ATTEMPTS
//
//  Backoff: 1s 2s 4s 8s 16s 30s → FAILED after MAX_ATTEMPTS
//
//  AshuraCore responsibilities:
//    - Call update() only when WiFi is CONNECTED
//    - Call resetForWifi() immediately when WiFi drops
//    - Call manualRetry() from Settings screen
//
//  NVS keys (namespace "network"):
//    net_ws_host  → server hostname or IP
//    net_ws_port  → server port  (default 3000)
//    net_ws_path  → server path  (default "/ws")
// ================================================================

enum class WebSocketState {
    IDLE,
    CONNECTING,
    CONNECTED,
    REGISTERED,
    FAILED
};

class WebSocketManager {
    public: 

        // ── Lifecycle ─────────────────────────────────────────────
        void init();
        void update();
        void resetForWifi();  // call immediately when WiFi drops

        // ── Manual actions ────────────────────────────────────────
        void manualRetry();   // Settings -> Retry
        void saveConfig(const String& host, int port, const String& path);

        // ── Send ──────────────────────────────────────────────────
        void send(const String& json);

        // ── State ─────────────────────────────────────────────────
        WebSocketState  webSocketState() const { return _state; }
        bool            isRegistered() const { return _state == WebSocketState::REGISTERED; }
        int             attemptCount() const { return _attempts; }
        String          host() const { return _host; }
        int             port() const { return _port; }
        String          path() const { return _path; }
        bool            hasConfig() const { return _host.length() > 0; }

        // ── NVS ───────────────────────────────────────────────────
        void loadConfig();

    private: 
        void                _beginConnect();
        void                _onFailed();
        void                _registerDevice();
        void                _sendHeartbeat();
        void                _onMessage(WebsocketsMessage message);
        void                _onEvent(WebsocketsEvent event, String data);
        unsigned long       _nextBackoff() const;

        WebsocketsClient    _client;
        WebSocketState      _state              = WebSocketState::IDLE;
        int                 _attempts           = 0;
        unsigned long       _retryAfter         = 0;   // millis() when next attempt allowed
        unsigned long       _attemptStart       = 0;   // millis() when _client.connect() fired
        bool                _attemptInFlight    = false;
        unsigned long       _lastHeartbeat      = 0;

        String              _host;
        int                 _port   = 3000;
        String              _path   = "/ws";
};