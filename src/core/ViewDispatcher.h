# pragma once
#include <Arduino.h>
#include <unordered_map>
#include <functional>
#include "../ui/screens/IScreen.h"

class ViewDispatcher {
    public:
        using CustomEventCb = std::function<bool(uint32_t event)>; // Handles event
        using NavigationEventCb = std::function<bool()>; // Handles navigation

        // ========================================================
        // View Management
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

        void setCustomEventCallback(CustomEventCb cb){
            _customEventCb = std::move(cb);
        }

        void setNavigationEventCallback(NavigationEventCb cb){
            _navigationEventCb = std::move(cb);
        }

        void sendCustomEvent(uint32_t event){
            if(_customEventCb) _customEventCb(event);
        }

        // ========================================================
        // Input Forwarding
        // ========================================================

        void onButtonUp() { if (_current) _current->onButtonUp(); }
        void onButtonDown() { if (_current) _current->onButtonDown(); }
        void onButtonSelect() { if (_current) _current->onButtonSelect(); }
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
        std::unordered_map<uint32_t, IScreen*> _views;
        IScreen* _current   = nullptr;
        uint32_t _currentId = UINT32_MAX;

        CustomEventCb        _customEventCb;
        NavigationEventCb    _navigationEventCb;
};