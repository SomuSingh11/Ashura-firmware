#pragma once
#include "IScreen.h"
#include "config.h"
#include "../../core/DisplayManager.h"
#include <Arduino.h>
#include <vector>
#include <functional>

// ============================================
// AppMenuScreen - OS App Launcher
//
// Grid-style menu with icon labels.
// Up/Down scrolls, Select launches app.
// Back returns to HomeScreen.
//
// Apps:
//   🕐 Clock     🐍 Games
//   ✨ Effects   📡 Network
//   ⚙  Settings
// ============================================

struct AppItem {
    String name;
    String icon;
    std::function<void()> onLaunch;
};

class AppMenuScreen : public IScreen {
    public:
        AppMenuScreen(DisplayManager& display, std::vector<AppItem> apps) : _display(display), _apps(std::move(apps)){}

        void onEnter() override {
            _cursor = 0;
            _dirty = true;
        }

        void onButtonDown() override {
            _cursor = (_cursor + 1) % _apps.size();
            _dirty = true;
        }   

        void onButtonUp() override {
            _cursor = (_cursor - 1 + _apps.size()) % _apps.size();
            _dirty = true;
        }

        void onButtonSelect() override {
            if(_cursor < (int)_apps.size() && _apps[_cursor].onLaunch) {
                _apps[_cursor].onLaunch();
            }
        }

        // Back is handled by UIManager (pops this screen -> returns to HomeScreen)

        void update() override {
            _display.clear();
            
            // ===== Draw Header =====
            _display.setFontLarge();
            _display.drawStr(0, 12, "Apps");
            _display.drawLine(0, 128, 12, 12);

            // ===== Draw App List =====
            _display.setFontMedium();
            int windowStart = (_cursor / UI_MAX_VISIBLE_ITEMS) * UI_MAX_VISIBLE_ITEMS; // Pagination

            for(int i=0; i<UI_MAX_VISIBLE_ITEMS; i++){
                int index = windowStart+i;
                if(index >= (int)_apps.size()) break;

                int y = UI_FIRST_ROW_Y + i * UI_ROW_HEIGHT;

                // ===== Cursor Indicator =====
                if(index == _cursor){
                    _display.drawStr(0, y, ">");
                    String label = "[" + _apps[index].icon + " " + _apps[index].name + "]";
                    _display.drawStr(10, y, label.c_str());
                } else {
                    String label = " " + _apps[index].icon + " " + _apps[index].name;
                    _display.drawStr(10, y, label.c_str());
                }
            }

            // ===== Scroll Hint =====
            if((int)_apps.size() > UI_MAX_VISIBLE_ITEMS){
                _display.setFontSmall();
                String hint = String(_cursor+1) + "/" + String(_apps.size());
                _display.drawStr(UI_HINT_PADDING, 63, hint.c_str());
            }

            // ===== Back Hint =====
            _display.setFontSmall();
            _display.drawStr(0, 63, "Back=Home");

            _display.sendBuffer();
            _dirty = false;
        }

    private:
        DisplayManager&         _display;
        std::vector<AppItem>    _apps;
        int                     _cursor = 0;
};