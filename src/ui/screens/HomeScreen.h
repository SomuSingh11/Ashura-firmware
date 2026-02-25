#pragma once
#include "IScreen.h"
#include "config.h"
#include "../../core/DisplayManager.h"
#include "../../core/TimeManager.h"
#include <Arduino.h>
#include <WiFi.h>
#include <time.h>


// ============================================
// HomeScreen - The OS Dashboard
//
// Shows:
//   - Large HH:MM clock center
//   - Ticking seconds bar at the bottom
//   - Connection badge top-right (● / ○)
//   - Last notification message
//   - IP address footer
//
// Encoder/Down press → signals app wants menu open
// Idle >SCREENSAVER_TIMEOUT → signals screensaver
// ============================================

class HomeScreen : public IScreen {
    public:
        explicit HomeScreen(DisplayManager& display) : _display(display) {};

        void onEnter() override {
            _lastInteraction = millis();
            _dirty = true;
        }

        bool needsContinuousUpdate() const override { return true; }

        // Called by AppManager to update connection status (e.g. on WiFi events)
        void setConnectionStatus(const String& s){
            _connectionStatus = s;
            _dirty = true;
        }

        void setLastMessage(const String& msg){
            _lastMessage = msg;
            _lastMessageTime = millis();
            _dirty = true;
        }

        // AppManager checks to push AppMenuScreen
        bool wantsMenu(){
            if(_wantsMenu) {
                _wantsMenu = false;
                return true;
            }
            return false;
        }

        // AppManager checks to push ScreenSaver
        bool wantsScreenSaver(){
            if(_wantsScreensaver){
                _wantsScreensaver = false;
                return true;
            }
            return false;
        }

        void onButtonSelect() override {
            _lastInteraction = millis();
            _wantsMenu = true;
        }

        void onButtonDown() override {
            _lastInteraction = millis();
            _wantsMenu = true;
        }

        void onButtonUp() override {
            _lastInteraction = millis();
        }

        void onButtonBack() override {
            _lastInteraction = millis();
        }

        void update() override {
            unsigned long now = millis();

            //Screensaver Trigger
            if(now - _lastInteraction > SCREENSAVER_TIMEOUT) {
                _wantsScreensaver = true;
                _lastInteraction = now;
            }

            _display.clear();

            // ===== Connection Badge (top-right) =====
            _display.setFontSmall();
            bool connected = (_connectionStatus == "[ * ]");
            _display.drawStr(100, 8, connected ? "[ * ]" : "[ - ]");

            // ===== App Name (top-left) =====
            _display.setFontSmall();
            _display.drawStr(0, 8, "Ashura Core");

            // ===== Clock HH:MM =====
            _display.setFontLarge();
            char timeBuf[6];
            sprintf(timeBuf, "%02d:%02d", Time().getHH(), Time().getMM());
            _display.drawStr(20, 38, timeBuf);

            // 60 dots across 120px, filled up to current second
            // ===== Seconds =====
            int ss = Time().getSS();
            _display.setFontSmall();
            char secBuf[4];
            sprintf(secBuf, ":%02d", ss);
            _display.drawStr(96, 38, secBuf);

            // Seconds progress bar (thin line at bottom of clock area)
            int barWidth = (ss * 128) / 60;
            _display.drawLine(0, barWidth, 44, 44);

            // ===== Uptime badge =====
            if (!Time().isSynced()) {
                _display.setFontSmall();
                _display.drawStr(80, 63, "[Uptime]");
            } else {
                _display.setFontSmall();
                _display.drawStr(80, 63, "[NTP]");
            }

            // ===== Last Message (fades after 10 sec) =====
            if(_lastMessage.length() > 0 && now-_lastMessageTime < LASTMESSAGE_HOMESCREEN_TIMEOUT){
                _display.setFontSmall();
                String display = _lastMessage;
                if(display.length() > 20) display = display.substring(0, 17) + "...";
                _display.drawStr(0, 54, display.c_str());
            }

            // ===== IP / Hint Footer =====
            _display.setFontSmall();
            if (_lastMessage.length() == 0 || millis() - _lastMessageTime > LASTMESSAGE_HOMESCREEN_TIMEOUT) {
                if (!Time().isSynced()) {
                    _display.drawStr(0, 63, "Syncing...");  // ← shown until NTP responds
                } else {
                    String ip = WiFi.status() == WL_CONNECTED
                        ? WiFi.localIP().toString()
                        : "No Network";
                    _display.drawStr(0, 63, ip.c_str());
                }
            }

            _display.sendBuffer();
        }
        
    
    private:
        DisplayManager&     _display;
        String              _connectionStatus = "[ - ]";
        String              _lastMessage = "";
        unsigned            _lastMessageTime = 0;
        unsigned            _lastInteraction = 0;
        bool                _wantsMenu = false;
        bool                _wantsScreensaver = false;
};