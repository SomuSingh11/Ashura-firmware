#pragma once
#include "IScreen.h"
#include "../../core/DisplayManager.h"
#include <WiFi.h>

// ============================================
// StatusScreen - The home / idle screen
// Shows connection state, status message, IP
// ============================================

class StatusScreen : public IScreen {
    public:
        explicit StatusScreen(DisplayManager& display): _display(display) {}

        void setConnectionStatus(const String& s) {
            _connectionStatus = s;
            markDirty();
        }
        void setMessage(const String& m){
            _message = m;
            markDirty();
        }

        void update() override {
            _display.clear();

            // Header
            _display.setFontLarge();
            _display.drawStr(0, 10, "Ashura Core");
            // _display.drawLine(0, 12, 128, 12);

            // Connection badge
            _display.setFontMedium();
            _display.drawStr(0, 25, _connectionStatus.c_str());

            // Status message
            _display.drawStr(0, 40, _message.c_str());

            // Footer - IP
            _display.setFontSmall();
            _display.drawStr(0, 60, WiFi.localIP().toString().c_str());

            _display.sendBuffer();
            _dirty = false;
        }
    
    private:
        DisplayManager& _display;
        String _connectionStatus = "[ Offline ]";
        String _message = "Starting...";
};