#include "MessageRouter.h"
#include <ArduinoJson.h>

//adds service to the list
void MessageRouter::registerService(IService* service){
    _services.push_back(service);
    Serial.println("📦 Service registered: " + String(service->getName()));
}

void MessageRouter::route(const String& json){
    JsonDocument doc;
    if(deserializeJson(doc, json) != DeserializationError::Ok){
        Serial.println("❌ MessageRouter: bad JSON");
        return;
    }

    for(auto* svc: _services){
        if (svc->handleMessage(doc)) {
            Serial.println("📬 Handled by: " + String(svc->getName()));
            return;
        }
    }

    Serial.println("⚠️ No service handled message type: " + String(doc["type"].as<const char*>()));
}