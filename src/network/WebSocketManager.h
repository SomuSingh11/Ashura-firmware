#pragma once
#include<Arduino.h>
#include<ArduinoWebsockets.h>

using namespace websockets;

// ============================================
// WebSocketManager
// Owns the WS client. Handles connect/reconnect/heartbeat.
// Publishes to EventBus; never touches UI directly.
// ============================================

class WebSocketManager {
    public: 
        void init();
        void connect();
        void update();
        void send(const String& json);

        bool isRegistered() const {return _registered;}

    private: 
        void _onMessage(WebsocketsMessage msg);
        void _onEvent(WebsocketsEvent event, String data);
        void _registerDevice();
        void _sendHeartbeat();

        WebsocketsClient        _client;
        bool                    _registered = false;
        unsigned long           _lastHeartbeat = 0;
        unsigned long           _lastReconnectAttempt=0;
};