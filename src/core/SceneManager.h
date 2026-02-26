#pragma once
#include <Arduino.h>
#include <vector>
#include <functional>

// ============================================================
// SceneManager
//
// Purpose:
// --------
// Manages navigation between UI scenes (screens).
// Each scene defines three lifecycle callbacks:
//
//   on_enter → called when scene becomes active
//   on_event → handles events while active
//   on_exit  → called before scene is left
//
// Features:
// ---------
// ✔ Stack-based navigation
// ✔ Scene lifecycle handling
// ✔ Per-scene state storage (_stateVec)
// ✔ Back navigation logic
// ✔ Generic & reusable design
// ============================================================

// ===== Callback types =====
using SceneEnterCb = std::function<void(void* ctx)>;
using SceneEventCb = std::function<bool(void* ctx, uint32_t event)>;
using SceneExitCb = std::function<void(void*  ctx)>;

// ===== Event Kinds sent to on_event =====
enum class SceneEventType : uint8_t {
    Custom,     // application-defined uint32_t
    Back,       // back-button pressed
    Tick,       // periodic tick
};

// ===== Scene Handler Table Entry =====
// Represents a single scene’s behaviour
struct SceneHandlerEntry {
    SceneEnterCb        on_enter; // Scene activation logic
    SceneEventCb        on_event; // Event processing logic
    SceneExitCb         on_exit;  // Cleanup logic
};

// ===== Scene Manager =====
class SceneManager {
    public:
        // Initialise with a fixed handler table and app context pointer
        // handlers → table of scene callbacks
        // ctx      → pointer to global application state
        void init(const std::vector<SceneHandlerEntry>& handlers, void* ctx){
            _handlers = handlers;
            _ctx = ctx;
            _stateVec.assign(handlers.size(), 0u);
        }

        // ========================================================
        // Navigation Functions
        // ========================================================

        // Push a new scene onto the stack (triggers exit on current, enter on new)
        void nextScene(uint32_t sceneId) {
            if(!_stack.empty()) _callExit(_stack.back());
            _stack.push_back(sceneId);
            _callEnter(sceneId);
        };

        // Pop the current scene (triggers exit + enter on previous)
        bool previousScene() {
            if(_stack.size() <= 1) return false;
            _callExit(_stack.back());
            _stack.pop_back();
            _callEnter(_stack.back());
            return true;
        }

        // Pop back to a specific scene (clears everything above it)
        bool searchAndSwitchToAnotherScene(uint32_t sceneId){
            for(int i=(int)_stack.size()-1; i>=0; --i){
                if(_stack[i] == sceneId) {
                    while(_stack.size() > (size_t)(i+1)){
                        _callExit(_stack.back());
                        _stack.pop_back();
                    }
                    _callEnter(sceneId);
                    return true;
                }
            }
            return false;
        }

        // ========================================================
        // Event Dispatch
        // ========================================================
        
        // Send an event to the current scene’s on_event callback
        bool handleCustomEvent(uint32_t event){
            if(_stack.empty()) return false;
            auto& handler = _handlers[_stack.back()];
            if(handler.on_event) return handler.on_event(_ctx, event);
            return false;
        }

        // Handle Back Action (ex: close dialog without closing parent scene)
        bool handleBackEvent() {
            if(_stack.empty()) return false;
            auto& handler = _handlers[_stack.back()];

            // Let scene try to handle back event
            bool consumed = handler.on_event ?
                                handler.on_event(_ctx, (uint32_t)SceneEventType::Back)
                                : false;
            
            // If not handled -> perform default navigation
            if(!consumed)
                return previousScene();

            // true  → Scene took responsibility
            // false → SceneManager takes responsibility
            return consumed;
        }

        // ========================================================
        // Per-Scene State Storage
        // ========================================================
        void setSceneState(uint32_t sceneId, uint32_t state){
            if(sceneId < _stateVec.size()){
                _stateVec[sceneId] = state;
            }
        }

        uint32_t getSceneState(uint32_t sceneId) const {
            return (sceneId < _stateVec.size()) ?
                _stateVec[sceneId]
                : 0;
        }

        // ===== Get current active scene
        uint32_t currentScene() const {
            return (_stack.empty()) ?
                UINT32_MAX
                :_stack.back();
        }

    private:
        // ===== Scene Lifecycle Helpers =====
        void _callEnter(uint32_t sceneId){
            if(sceneId < _handlers.size() && _handlers[sceneId].on_enter)
                _handlers[sceneId].on_enter(_ctx);
        };
        void _callExit(uint32_t sceneId){
            if(sceneId < _handlers.size() && _handlers[sceneId].on_exit)
                _handlers[sceneId].on_exit(_ctx);
        };

        std::vector<SceneHandlerEntry>  _handlers;       // Scene definitions
        std::vector<uint32_t>           _stack;          // Navigation Stack
        std::vector<uint32_t>           _stateVec;       // Per-scene Memory
        void*                           _ctx = nullptr;  // App context
};  