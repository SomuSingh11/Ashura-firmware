#include "WebSocketManager.h"
#include "config.h"

#include <WiFi.h>
#include <ArduinoJson.h>
#include <Preferences.h>

#include "../core/EventBus.h"
#include "../core/NotificationManager.h"

// ================================================================
//  WebSocketManager.cpp
// ================================================================

// ── Lifecycle ─────────────────────────────────────────────────

// Configure callbacks for messages and events
void WebSocketManager::init(){
    loadConfig();

    _client.onMessage([this](WebsocketsMessage msg){
        _onMessage(msg);
    });
    _client.onEvent([this](WebsocketsEvent ev, String data) {
        _onEvent(ev, data);
    });
    
    Serial.println("[WS] Ready — " + _host + ":" + String(_port) + _path);
}

void WebSocketManager::update() {
    switch(_state){

        // ── Waiting for WiFi — AshuraCore only calls us when WiFi up ──
        case WebSocketState::IDLE:
            if(hasConfig()){
                Serial.println("[WS] WiFi up, starting connection");
                _state              = WebSocketState::CONNECTING;
                _attemptInFlight    = false;
                _retryAfter         = 0;
            }
            break;

        case WebSocketState::CONNECTING: {
            unsigned long now = millis();

            // 1. Sitting out backoff delay
            if(now < _retryAfter) break;

            // 2. Attempt in flight — check for timeout
            if(_attemptInFlight){
                if(now - _attemptStart < Config::WebSocket::CONNECT_TIMEOUT) break;

                // Timed out — onEvent(ConnectionOpened) never fired
                _attemptInFlight = false;
                _attempts++;
                Serial.println("[WS] Attempt " + String(_attempts) +
                               "/" + String(Config::WebSocket::MAX_ATTEMPTS) + " timed out");
                
                if(_attempts >= Config::WebSocket::MAX_ATTEMPTS){
                    _onFailed();
                } else {
                    unsigned long delay = _nextBackoff();
                    _retryAfter = now + delay;
                    Serial.println("[WS] Backoff " + String(delay / 1000) + "s");
                }
                break;
            }

            // 3. Nothing in flight, backoff elapsed — fire attempt
            Serial.println("[WS] Attempt " + String(_attempts + 1) +
                           "/" + String(Config::WebSocket::MAX_ATTEMPTS));
            _beginConnect();
            break;
        }

        // ── Socket open, waiting for registration ──────────────
        case WebSocketState::CONNECTED:
            if(_client.available()){
                _client.poll();
            } else {
                // Dropped before registration
                Serial.println("[WS] Dropped before registration");
                _attempts++;
                _attemptInFlight = false;

                if(_attempts >= Config::WebSocket::MAX_ATTEMPTS){
                    _onFailed();
                } else {
                    unsigned long wait = _nextBackoff();
                    _retryAfter = millis() + wait;
                    _state      = WebSocketState::CONNECTING;
                    Bus().publish(AppEvent::WebSocketDisconnected);
                }
            }
            break;
            

        // ── Fully operational ──────────────────────────────────
        case WebSocketState::REGISTERED: 
            if(_client.available()) {
                _client.poll();

                // Heartbeat
                unsigned long now = millis();
                if(now - _lastHeartbeat > HEARTBEAT_INTERVAL){
                    _lastHeartbeat = now;
                    _sendHeartbeat();
                }
            } else {
                // Dropped — restart with fresh backoff
                Serial.println("[WS] Connection dropped");
                _state           = WebSocketState::CONNECTING;
                _attempts        = 0;
                _attemptInFlight = false;
                _retryAfter      = 0;
                Bus().publish(AppEvent::WebSocketDisconnected);
            }
            break;

            
        // ── Gave up — waiting for manualRetry() ───────────────
        case WebSocketState::FAILED:
            break;
    }
}

void WebSocketManager::resetForWifi() {
    if(_client.available()) _client.close();
    _state              = WebSocketState::IDLE;
    _attempts           = 0;
    _retryAfter         = 0;
    _attemptInFlight    = false;
    Serial.println("[WS] Reset — WiFi dropped");
}


// ── Manual actions ────────────────────────────────────────────

void WebSocketManager::manualRetry() {
    if(!hasConfig()){
        Serial.println("[WS] manualRetry — no config");
        return;
    }

    Serial.println("[WS] Manual retry");
    _attempts           = 0;
    _attemptInFlight    = false;
    _retryAfter         = 0;
    _state              = WebSocketState::CONNECTING;
}

void WebSocketManager::saveConfig(const String& host, int port, const String& path){
    _host = host;
    _port = port;
    _path = path;

    Preferences p;
    p.begin("network", false);
    p.putString("net_ws_host", host);
    p.putInt("net_ws_port", port);
    p.putString("net_ws_path", path);
    p.end();

    Serial.println("[WS] Config saved: " + host + ":" +
                   String(port) + path);

    _attempts           = 0;
    _attemptInFlight    = false;
    _retryAfter         = 0;
    _state              = WebSocketState::CONNECTING;
}

void WebSocketManager::send(const String& json){
    if (_state == WebSocketState::REGISTERED || _state == WebSocketState::CONNECTED) {
        _client.send(json);
    } else {
        Serial.println("[WS] send() dropped — not connected");
    }
}


// ── NVS ───────────────────────────────────────────────────────

void WebSocketManager::loadConfig() {
    Preferences p;
    p.begin("network", true);
    _host = p.getString("net_ws_host", "");
    _port = p.getInt("net_ws_port", 3000);
    _path = p.getString("net_ws_path", "/ws");
    p.end();

    if (_host.length() > 0) {
        Serial.println("[WS] Config: " + _host + ":" + String(_port) + _path);
    } else {
        Serial.println("[WS] No server config saved");
    }
}


// ── Private ───────────────────────────────────────────────────

void WebSocketManager::_beginConnect() {
    String url = "ws://" + _host + ":" + String(_port) + _path;
    Serial.println("[WS] Connecting to: " + url);

    _attemptStart = millis();
    _attemptInFlight = true;

    // _client.connect() fires _onEvent(ConnectionOpened) synchronously if
    // successful — which sets _state = CONNECTED and clears _attemptInFlight.
    // If it returns false the attempt stays in flight and times out normally.
    _client.connect(url);
}

void WebSocketManager::_onFailed() {
    _state           = WebSocketState::FAILED;
    _attemptInFlight = false;

    if (_client.available()) _client.close();
    Serial.println("[WS] FAILED after " + String(Config::WebSocket::MAX_ATTEMPTS) + " attempts");

    NotifMgr().push(
        NotificationType::SYSTEM,
        "Server Unavailable",
        "Could not connect after " + String(Config::WebSocket::MAX_ATTEMPTS) +
        " attempts. Go to Settings > Network to retry."
    );

    Bus().publish(AppEvent::WebSocketFailed);
}

void WebSocketManager::_registerDevice(){
    JsonDocument doc;

    doc["type"]           = "register";
    doc["deviceId"]       = DEVICE_ID;
    doc["deviceType"]     = "esp32";
    doc["data"]["name"]   = DEVICE_NAME;
    doc["data"]["ip"]     = WiFi.localIP().toString();
    doc["data"]["mac"]    = WiFi.macAddress();
    doc["timestamp"]      = millis();

    String msg;
    serializeJson(doc, msg);
    _client.send(msg);
    Serial.println("[WS] Registration sent");
}

void WebSocketManager::_sendHeartbeat() {
    JsonDocument doc;
    doc["type"]      = "heartbeat";
    doc["deviceId"]  = DEVICE_ID;
    doc["timestamp"] = millis();

    String msg;
    serializeJson(doc, msg);
    _client.send(msg);
    Serial.println("[WS] Heartbeat");
}

void WebSocketManager::_onMessage(WebsocketsMessage msg) {
    Serial.println("[WS] RX: " + msg.data());

    JsonDocument doc;
    if(deserializeJson(doc, msg.data()) != DeserializationError::Ok){
        Serial.println("[WS] RX: Invalid JSON");
        return;
    }

    const char* type = doc["type"];
    if(!type) return;

    if(strcmp(type, "status") == 0){
        const char* status = doc["data"]["status"];

        if(status && strcmp(status, "registered") == 0){
            Serial.println("[WS] Registered");
            _state      = WebSocketState::REGISTERED;
            _attempts   = 0;
            _lastHeartbeat = millis();
            Bus().publish(AppEvent::WebSocketRegistered);
        }
    } else if( strcmp(type, "command") == 0 || 
               strcmp(type, "notification") == 0) {
        _lastHeartbeat = millis();
        Bus().publish(AppEvent::CommandReceived, msg.data());
    }
}

void WebSocketManager::_onEvent(WebsocketsEvent event, String data) {
    switch(event) {
        case WebsocketsEvent::ConnectionOpened:
            Serial.println("[WS] Socket opened");
            _state              = WebSocketState::CONNECTED;
            _attemptInFlight    = false;
            _attempts           = 0;

            Bus().publish(AppEvent::WebSocketConnected);
            _registerDevice();
            break;

        case WebsocketsEvent::ConnectionClosed:
            Serial.println("[WS] Socket closed");  
            // Let update() handle state transition on next tick
            // — avoids calling state machine logic from a callback
            break;  

        case WebsocketsEvent::GotPing:
        case WebsocketsEvent::GotPong:
            break;
    }
}

unsigned long WebSocketManager::_nextBackoff() const {
   unsigned long b = Config::WebSocket::BACKOFF_BASE;
   for(int i=0; i<_attempts && b < Config::WebSocket::BACKOFF_CAP; i++) b *= 2;
   return min(b, Config::WebSocket::BACKOFF_CAP);
}