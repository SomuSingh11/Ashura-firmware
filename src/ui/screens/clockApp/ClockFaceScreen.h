#pragma once
#include "../../DisplayManager.h"
#include "../IScreen.h"
#include "../../../core/TimeManager.h"

class ClockFaceScreen : public IScreen {
    public:
        explicit ClockFaceScreen(DisplayManager& display) : _display(display) {};

        void onEnter() override {
            _dirty = true;
        }

        bool needsContinuousUpdate() const override { return true; }

        // onButtonBack is handled by UI_Manager (pop screen)

        void update() override {
            int ss = Time().getSS();
            int mm = Time().getMM();
            int hh = Time().getHH();
            
            _display.clear();

            // Title
            _display.setFontSmall();
            _display.drawStr(45, 8, "Clock");

            // HH:MM
            _display.setFontLarge();
            char buf[6];
            sprintf(buf, "%02d:%02d", hh, mm);
            _display.drawStr(18, 35, buf);

            // Seconds (small, bottom-right)
            _display.setFontSmall();
            char sBuf[4];
            sprintf(sBuf, "%02d", ss);
            _display.drawStr(92, 35, sBuf);

            // Seconds Progress Bar 
            int barX = (ss * 128) / 60;
            _display.drawLine(0, barX, 45, 45);

            // Uptime Label
            _display.setFontSmall();
            if(Time().isSynced()){
                _display.drawStr(0, 60, "Time: NTP Synced");
            } else {
                _display.drawStr(0, 60, "Time: Uptime");
            }

            _display.drawStr(90, 63, "[Back]");
            _display.sendBuffer();
        }

    private:
        DisplayManager& _display;
};