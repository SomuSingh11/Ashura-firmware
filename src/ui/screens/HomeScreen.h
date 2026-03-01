#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <time.h>

#include "IScreen.h"
#include "config.h"

#include "../../companion/CompanionRenderer.h"
#include "../../core/DisplayManager.h"
#include "../../core/TimeManager.h"

// ================================================================
//  HomeScreen  —  Ashura OS Desktop
//
//  128×64 layout:
//
//  ┌──────────────────────────────────────────────────────────────┐
//  │                                           Mon 23 Jan    [*] │  y=8
//  │                                                             │
//  │                                           14:32             │  y=28
//  │                                           :45               │  y=38
//  │                                           ████████████████  │  seconds bar y=45
//  │  ╭──╮   ╭──╮                                               │
//  │  │● │   │ ●│    ←  companion eyes  48×26  bottom-left      │
//  │  ╰──╯   ╰──╯                                               │
//  └──────────────────────────────────────────────────────────────┘
//
//  Companion region: x=0 y=38 w=56 h=26
//  Clock region:     right half  x=62
//  Date + badge:     top-right   y=8
//
//  SELECT / DOWN → wantsMenu()
//  Idle > SCREENSAVER_TIMEOUT → wantsScreenSaver()
// ================================================================

static const char* const _dayName[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
static const char* const _monthName[] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
     "Jul","Aug", "Sep", "Oct", "Nov", "Dec"
};

class HomeScreen : public IScreen {
    public:
        explicit HomeScreen(DisplayManager& display, CompanionRenderer& companion) : _display(display), _companion(companion) {};

        void onEnter() override {
            _lastInteraction    = millis();
            _dirty              = true;
        }

        bool needsContinuousUpdate() const override { return true; }

        // ── Called by AshuraCore event wires ────────────────────
        void setConnectionStatus(const String& s){
            _connectionStatus = s;
        }
        void setLastMessage(const String& msg){
            _lastMessage = msg;
            _lastMessageAt = millis();
        }

        // ── Signals polled by AshuraCore ─────────────────────────
        bool wantsMenu(){
            if(_wantsMenu) {
                _wantsMenu = false;
                return true;
            }
            return false;
        }
        bool wantsScreenSaver(){
            if(_wantsScreensaver){
                _wantsScreensaver = false;
                return true;
            }
            return false;
        }

        // ── Buttons ──────────────────────────────────────────────
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

        // ── Render ───────────────────────────────────────────────
        void update() override {
            unsigned long now = millis();

            // ScreenSaver trigger
            if(now - _lastInteraction > SCREENSAVER_TIMEOUT){
                _wantsScreensaver   = true;
                _lastInteraction    = now;
            }

            // Always redraw - companion animates continuously
            auto& u = _display.raw(); //U8G2 object
            u.clearBuffer();
            
            // ── Companion eyes — bottom-left 56×26 ───────────────
            // Region: x=0, y=38, w=56, h=26
            _companion.draw(u, 0, 38, 56, 26);

            // ── Date line (top-right, y=8) ────────────────────────
            u.setFont(u8g2_font_5x7_tr);
            if(Time().isSynced()){
                struct tm t;
                if(getLocalTime(&t, 0)){
                    char buf[14];
                    sniprintf(buf,  sizeof(buf), "%s %02d %s",
                        _dayName[t.tm_wday],t.tm_mday, _monthName[t.tm_mon]);
                    u.drawStr(62, 8, buf);
                }
            } else {
                u.drawStr(62, 8, "Syncing...");
            }

            // ── Connection badge (top-right corner) ──────────────
            bool online = _connectionStatus == "[ * ]";
            u.drawStr(121, 8, online ? "*" : "-");

            // ── HH:MM (large, right half) ─────────────────────────
            u.setFont(u8g2_font_ncenB08_tr);
            char timeBuf[6];
            sniprintf(timeBuf, sizeof(timeBuf), "%02d:%02d",
               Time().getHH(), Time().getMM());
            u.drawStr(62, 32,timeBuf); 

            // ── Seconds ───────────────────────────────────────────
            u.setFont(u8g2_font_5x7_tr);
            char secBuf[4];
            snprintf(secBuf, sizeof(secBuf), ":%02d", Time().getSS());
            u.drawStr(62, 42, secBuf);

            // ── Seconds progress bar ──────────────────────────────
            int barW = (Time().getSS() * 60) / 60;
            u.drawBox(62, 45, barW, 2);

            // ── Last notification (bottom strip, fades after timeout)
            if (!_lastMessage.isEmpty() &&
                now - _lastMessageAt < (unsigned long)LASTMESSAGE_HOMESCREEN_TIMEOUT)
            {
                u.setFont(u8g2_font_5x7_tr);
                String m = _lastMessage;
                if (m.length() > 21) m = m.substring(0, 18) + "...";
                u.drawStr(0, 63, m.c_str());
            }

            u.sendBuffer();
            _dirty = false;
        }
        
    
    private:
        DisplayManager&     _display;
        CompanionRenderer&   _companion;

        String              _connectionStatus  = "[ - ]";
        String              _lastMessage;
        unsigned            _lastMessageAt   = 0;
        unsigned            _lastInteraction   = 0;
        bool                _wantsMenu         = false;
        bool                _wantsScreensaver  = false;
};