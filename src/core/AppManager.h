#pragma once
#include "WiFiManager.h"
#include "../network/WebSocketManager.h"
#include "../network/MessageRouter.h"
#include "../ui/UIManager.h"
#include "../ui/DisplayManager.h"
#include "../services/DeviceService.h"
#include "../ui/screens/HomeScreen.h"

// ============================================
// AppManager - Top-level Orchestrator
//
// Boot sequence:
//   1. Display init
//   2. Push SplashScreen (auto-transitions)
//   3. Push HomeScreen (dashboard, permanent base)
//   4. Wire all EventBus subscriptions
//   5. Connect WiFi + WebSocket (background)
//
// Update loop:
//   - Poll WebSocket
//   - Tick UI (handles games, animations, dirty redraws)
//   - Check HomeScreen signals (menu, screensaver)
//   - Read physical buttons
// ============================================

class AppManager {
    public: 
        void init();
        void update();

        struct BtnState {
            bool            lastRaw         = HIGH; // Last raw reading from pin
            bool            state           = HIGH; // Debounced state
            unsigned long   lastDebounce    = 0; // Timestamp of last state change for debounce
        };

    private:
        // =========================================
        // Internal Helpers
        // =========================================
        void _wireEvents(); // Subscribes modules to EventBus | Connects Subsystems
        void _buildAppMenu(); // Constructs AppMenuScreen and pushes it
        void _lauchScreenSaver(); // Pushes ScreenSaverScreen 
        bool _readButton(int pin, struct BtnState& state); // Reads button state with debouncing

        // =========================================
        // Subsystems (Owned by OS)
        // =========================================
        WiFiManager         _wifi;             // Handles WiFi connection
        DisplayManager      _display;          // Handles screen rendering
        UIManager           _ui;               // Manages UI state and rendering
        WebSocketManager    _websocket;        // Manages WebSocket connection
        MessageRouter       _router;           // Routes incoming messages to correct handler
        DeviceService       _deviceService;  

        // =========================================
        // OS State
        // =========================================
        HomeScreen*         _homeScreen             = nullptr;      // Pointer to HomeScreen for status updates
        String              _websocketStatus        = "Offline";    // Tracks WebSocket status for display
        unsigned long       _websocketConnectedAt   = 0;            // For throttling WebSocket reconnects
        bool                _networkInitDone        = false;        // Flag to track if initial network setup is complete

        // =========================================
        // Button Debounce State
        // =========================================
        BtnState _btnUp;
        BtnState _btnDown;
        BtnState _btnSelect;
        BtnState _btnBack;
};