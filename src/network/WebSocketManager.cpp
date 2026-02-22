#include "WebSocketManager.h"
#include "../core/EventBus.h"
#include <WiFi.h>
#include <ArduinoJson.h>
#include "config.h"

// Configure callbacks for messages and events
void WebSocketManager::init(){
    _client.onMessage([this](WebsocketsMessage msg){
        _onMessage(msg);
    });
    _client.onEvent([this](WebsocketsEvent ev, String data) {
        _onEvent(ev, data);
    });
    Serial.print("✅WebSocket handlers configured");
}

// Connect to the WebSocket server and trigger device registration
void WebSocketManager::connect(){
    if(WiFi.status() != WL_CONNECTED){
        Serial.println("⚠️ WiFi not connected, skipping WS connect");
        return;
    }

    String url = "ws://" + String(WS_SERVER_HOST) + ":" + String(WS_SERVER_PORT) + String(WS_SERVER_PATH);
    Serial.println("🔌 Connecting to: " + url);

    if(_client.connect(url)){
        Serial.println("✅ WebSocket connected!");
        delay(500);
        _registerDevice();
    } else {
        Serial.println("❌ WebSocket connection failed!");
        Bus().publish(AppEvent::WebSocketDisconnected);
    }
}

// Handle incoming messages, reconnect if needed, and send heartbeat
void WebSocketManager::update(){
    if(_client.available()){
        _client.poll(); // processes incoming messages and triggers callbacks (like _onMessage)
    }

    //try reconnect
    if(!_client.available()){
        unsigned long now = millis();
        if (now - _lastReconnectAttempt > RECONNECT_DELAY) {
            _lastReconnectAttempt = now;
            _registered = false;
            connect();
        }
    }

    //send heartbeat
    if (_registered) {
        unsigned long now = millis();
        if (now - _lastHeartbeat > HEARTBEAT_INTERVAL) {
            _lastHeartbeat = now;
            _sendHeartbeat();
        }
    }
}

// Send a raw JSON string over the WebSocket
void WebSocketManager::send(const String& json){
    _client.send(json);
}


// ---------------- Private ----------------

// Process incoming WebSocket messages (status, command, notification)
void WebSocketManager::_onMessage(WebsocketsMessage msg){
    Serial.println("\n📨 WebSocket RX: " + msg.data());

    JsonDocument doc;
    if(deserializeJson(doc, msg.data()) != DeserializationError::Ok){
        Serial.println("❌ JSON parse error");
        return;
    }

    const char* type = doc["type"];
    if(!type){
        Serial.println("❌ No type field in message");
        return;
    }

    if(strcmp(type, "status") == 0){
        const char* status = doc["data"]["status"];
        if(!status){
            Serial.println("❌ Missing status field");
            return;
        }

        //registered
        else if(strcmp(status, "registered") == 0){
            Serial.println("✅ Device Registered!");
            _registered = true;
            _lastHeartbeat = millis();
            Bus().publish(AppEvent::WebSocketRegistered);
        }

        //error
        else if(strcmp(status, "error") == 0){
            Serial.println("❌ Server reported error status");
        }

        else {
            Serial.println("ℹ️ Status: " + String(status));
        }
    } else if (strcmp(type, "command") == 0){
        Serial.println("📥 Command received");
        _lastHeartbeat = millis();
        Bus().publish(AppEvent::CommandReceived, msg.data());
    } 
    
    else if(strcmp(type, "notification") == 0) {
        Serial.println("🔔 Notification received");
        Bus().publish(AppEvent::CommandReceived, msg.data());
    } 
    
    else {
        Serial.println("⚠️ Unknown message type: " + String(type));
    }
}

// Handle WebSocket events (open, close, ping/pong)
void WebSocketManager::_onEvent(WebsocketsEvent event, String data){
    switch(event){
        case WebsocketsEvent::ConnectionOpened:
            Serial.println("✅ WebSocket opened");
            Bus().publish(AppEvent::WebSocketConnected);
            break;

        case WebsocketsEvent::ConnectionClosed:
            Serial.println("❌ WebSocket closed");
            _registered = false;
            Bus().publish(AppEvent::WebSocketDisconnected);
            break;

        case WebsocketsEvent::GotPing:
        case WebsocketsEvent::GotPong:
            break;
    }
}

// Send device registration data to the server
void WebSocketManager::_registerDevice(){
    JsonDocument doc;
    doc["type"]            = "register";
    doc["deviceId"]        = DEVICE_ID;
    doc["deviceType"]      = "esp32";
    doc["data"]["name"]    = DEVICE_NAME;
    doc["data"]["ip"]      = WiFi.localIP().toString();
    doc["data"]["mac"]     = WiFi.macAddress();
    doc["timestamp"]       = millis();

    String msg;
    serializeJson(doc, msg);
    _client.send(msg);
    Serial.println("📝 Registration sent");
}

// Send periodic heartbeat to indicate device is alive
void WebSocketManager::_sendHeartbeat() {
  JsonDocument doc;
  doc["type"]      = "heartbeat";
  doc["deviceId"]  = DEVICE_ID;
  doc["timestamp"] = millis();

  String msg;
  serializeJson(doc, msg);
  _client.send(msg);
  Serial.println("💓 Heartbeat");
}