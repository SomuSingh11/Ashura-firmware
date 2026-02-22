#pragma once
#include "../../DisplayManager.h"
#include "../IScreen.h"

class ClockFaceScreen : public IScreen {
    public:
        explicit ClockFaceScreen(DisplayManager& display) : _display(display) {};

        void onEnter() override {
            _startMs = millis();
            _dirty = true;
        }

        bool needsContinuousUpdate() const override { return true; }

        // onButtonBack is handled by UI_Manager (pop screen)

        void update() override {
            unsigned long elapsed = millis() - _startMs;
            int ss = (elapsed / 1000) % 60;
            int mm = (elapsed / 60000) % 60;
            int hh = (elapsed / 3600000) % 24;
            
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

            // Seconds Arc
            int barX = (ss * 128) / 60;
            _display.drawLine(0, barX, 45, 45);

            // Uptime Label
            _display.setFontSmall();
            _display.drawStr(0, 60, "Uptime:");
            _display.drawStr(90, 63, "[Back]");

            _display.sendBuffer();
        }

    private:
        DisplayManager& _display;
        unsigned long _startMs = 0;
};