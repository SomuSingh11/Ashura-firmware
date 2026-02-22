#pragma once
#include <vector>
#include "./services/IService.h"
#include <Arduino.h>

// ============================================
// MessageRouter
// Receives raw JSON strings from EventBus,
// fans them out to registered services in order.
// First service to return true "consumes" the message.
//
// Adding a new service = one line in AppManager::init()
// ============================================

class MessageRouter {
    public:
        void registerService(IService* service);
        void route(const String& json);
    
    private:
        std::vector<IService*> _services;
};