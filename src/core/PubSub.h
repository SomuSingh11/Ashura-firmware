#pragma once
#include <Arduino.h>
#include <functional>
#include <vector> 
#include <algorithm> // std::remove_if (unsubscribe logic)

/*
============================================================

 PubSub<T>  —  Typed Publish / Subscribe Channel

 Purpose:
   Provides a strongly-typed event channel where multiple
   subscribers can listen for events of type T.

 Why this exists:
   - Decouples modules (no direct dependencies)
   - Eliminates fragile string-based messaging
   - Compile-time type safety
   - Very lightweight OS-style signaling

 Core Concept:
   Each PubSub<T> instance represents ONE event channel.

   Example:
     PubSub<WifiEvent> wifiEvents;
     auto handle = wifiPubSub.subscribe([](const WifiEvent& e){ ... });
     wifiPubSub.publish({WifiEvent::Connected, "192.168.1.2"});
     wifiPubSub.unsubscribe(handle);

   This channel ONLY carries WifiEvent objects.

============================================================
*/

// Unique ID for each subscription
// Required later to safely unsubscribe.
using SubscriptionHandle = uint32_t; 

// ============================================================
// PubSub<T> — Generic Typed Event Channel
// ============================================================

// Template class → works with ANY event type T
template<typename T>

class PubSub {
    public:
        using Callback = std::function<void(const T&)>; // type alias

        // Subscribe → register callback → returns unique handle
        SubscriptionHandle subscribe(Callback cb){
            SubscriptionHandle h = _nextHandle++;
            _subs.push_back({h, std::move(cb)});
            return h;
        }

        // Unsubscribe → remove subscriber by handle
        void unsubscribe(SubscriptionHandle handle){
            _subs.erase(
                std::remove_if(_subs.begin(), _subs.end(),
            [handle](const Entry& e){
                return e.handle == handle;
            }),
            _subs.end());
        }

        // Publish → notify ALL subscribers
        void publish(const T& event) {
            for(auto& e: _subs) e.cb(event);
        }
    private:
        // Subscriber record
        struct Entry {
            SubscriptionHandle  handle;
            Callback            cb;
        };

        std::vector<Entry>  _subs;
        SubscriptionHandle  _nextHandle = 1;
};