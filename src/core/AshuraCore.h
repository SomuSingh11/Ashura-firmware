#pragma once
#include "WiFiManager.h"
#include "../network/WebSocketManager.h"
#include "../network/MessageRouter.h"
#include "../core/UIManager.h"
#include "../core/DisplayManager.h"
#include "../services/DeviceService.h"
#include "../core/Loader.h"
#include "../ui/screens/HomeScreen.h"
#include "../companion/MoodEngine.h"

// ============================================================
//  AshuraCore  —  Top-level OS kernel
//
//  Boot sequence:
//    1.  Serial / debug
//    2.  Display service  → record_create(RECORD_DISPLAY)
//    3.  GUI / UIManager  → record_create(RECORD_GUI)
//    4.  Loader service   → record_create(RECORD_LOADER)  
//    5.  Register all apps with Loader                    
//    6.  Push boot screens (Splash → Home)
//    7.  Wire EventBus subscriptions
//    8.  Button GPIO init
//    9.  WiFi service     → record_create(RECORD_WIFI)
//    10. NTP sync
//    11. WebSocket service→ record_create(RECORD_WEBSOCKET)
//    12. Register MessageRouter services
//    13. WebSocket connect
//
//  Update loop (mirrors Flipper's OS tick):
//    - WiFi watchdog
//    - NTP retry
//    - WebSocket poll
//    - UI tick (ViewDispatcher / UIManager)
//    - HomeScreen signals
//    - Physical button debounce
// ============================================================

class AshuraCore {
    public: 
        void init();
        void update();

        // ========================================================
        // Button Debounce State
        // ======================================================== 
        struct BtnState {
            bool            lastRaw         = HIGH; // Last raw reading from pin
            bool            state           = HIGH; // Debounced state
            unsigned long   lastDebounce    = 0;    // Timestamp of last state change for debounce
        };

    private:
        // ========================================================
        // Boot Helpers
        // ========================================================
        void _initServices();
        void _registerApps();
        void _bootUI();
        void _wireEvents();
        void _initButtons();
        void _initNetwork();

        // ========================================================
        // Runtime Helpers
        // ========================================================
        void _buildAppMenu();
        void _launchScreenSaver();
        bool _readButton(int pin, BtnState& state);

        // ========================================================
        // Services (owned by OS, registered in AshuraRecord)
        // ========================================================
        DisplayManager      _display;
        UIManager           _ui;
        WiFiManager         _wifi;
        WebSocketManager    _websocket;
        MessageRouter       _router;
        DeviceService       _deviceService;
        Loader              _loader;

        // ========================================================
        // Companion Subsystems
        // ========================================================
        MoodEngine          _mood;
        CompanionRenderer   _companion{_mood};

        // ========================================================
        // ASHURA CORE State
        // ========================================================
        HomeScreen*     _homeScreen         = nullptr;
        String          _wsStatus           = "Offline";
        bool            _networkInitDone    = false;

        // ========================================================
        // Button States
        // ========================================================
        BtnState _btnUp;
        BtnState _btnDown;
        BtnState _btnSelect;
        BtnState _btnBack;
};