# pragma once
#include <Arduino.h>
#include <unordered_map>
#include <functional>
#include "../ui/screens/IScreen.h"

// ============================================================
// ViewDispatcher
// ============================================================
//
// Purpose:
// --------
// Controls which screen (view) is currently active.
//
// Responsibilities:
// -----------------
// ✔ Stores all available screens
// ✔ Switches between screens
// ✔ Sends button input to active screen
// ✔ Calls update() when screen needs redraw
// ✔ Forwards events to SceneManager
// ============================================================

class ViewDispatcher {
    public:
        // Custom Event Callback
        // Used to forward events (usually to SceneManager)
        using CustomEventCb = std::function<bool(uint32_t event)>; 
        
        // Navigation Event Callback
        // Used for BACK navigation logic
        using NavigationEventCb = std::function<bool()>;

        // ========================================================
        // View Management "dispatcher.addView(0, &homeScreen)"
        // ========================================================

        void addView(uint32_t viewId, IScreen*  view){
            _views[viewId] = view;
        }

        void removeView(uint32_t viewId){
            if(_currentId == viewId) _current = nullptr;
            _views.erase(viewId);
        }

        // ========================================================
        // Navigation
        // ========================================================
        
        void switchTo(uint32_t viewId){
            auto it = _views.find(viewId);
            if(it == _views.end()) return;

            if(_current) _current->onExit();
            _current = it->second;
            _currentId = viewId;
            _current->onEnter();
            _current->markDirty();
        }

        // ========================================================
        // Event Callbacks (Wired to SceneManager)
        // ========================================================
        
        // Handle Custom Event (ex: SceneEventType::Custom)
        // dispatcher.setCustomEventCallback(sceneManager.handleCustomEvent);
        void setCustomEventCallback(CustomEventCb cb){
            _customEventCb = std::move(cb);
        }

        // Handle Navigation Event (ex: Scene wants to pop itself)
        // dispatcher.setNavigationEventCallback(sceneManager.handleBackEvent);
        void setNavigationEventCallback(NavigationEventCb cb){
            _navigationEventCb = std::move(cb);
        }

        // Send a custom event (ex: SceneEventType::Custom) to the SceneManager
        // dispatcher.sendCustomEvent(EVENT_MENU_OPEN);
        void sendCustomEvent(uint32_t event){
            if(_customEventCb) _customEventCb(event);
        }

        // ========================================================
        // Input Forwarding (ONLY the active screen receives button input)
        // ========================================================
        
        // This prevents:
        // ❌ Multiple screens reacting to same button
        // ❌ Global input chaos
        void onButtonUp() { if (_current) _current->onButtonUp(); }
        void onButtonDown() { if (_current) _current->onButtonDown(); }
        void onButtonSelect() { if (_current) _current->onButtonSelect(); }

        // Flow:
        // 1️⃣ Ask navigation system first (SceneManager)
        // 2️⃣ If NOT handled → let screen decide
        void onButtonBack() {
            // Give the navigation callback first refusal (SceneManager.handkeBackEvent) 
            bool consumed = _navigationEventCb ? _navigationEventCb() : false;
            if (!consumed && _current) _current->onButtonBack(); 
        }

        // ========================================================
        // Update / Render
        // ========================================================

        void update() {
            if(!_current) return;
            if(_current->needsContinuousUpdate()){
                _current->update();
                if(_current->wantsPop()){
                    if(_navigationEventCb) NavigationEventCb();
                }
            } else if (_current->isDirty()){
                _current->update();
            }
        }

        // ===== Retrieve Current View =====
        IScreen* currentView() const { return _current; }

    private:
        std::unordered_map<uint32_t, IScreen*> _views; // Stores all registered screens
        IScreen* _current   = nullptr;
        uint32_t _currentId = UINT32_MAX;

        CustomEventCb        _customEventCb;        // Event forwarding callback
        NavigationEventCb    _navigationEventCb;    // Navigation/back callback
};