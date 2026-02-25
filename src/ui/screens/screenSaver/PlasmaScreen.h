#pragma once
#include "config.h"
#include "../IScreen.h"
#include "../../../core/DisplayManager.h"

class PlasmaScreen : public IScreen {
public:
    explicit PlasmaScreen(DisplayManager& display) : _display(display) {}

    void onEnter() override { _t = 0; _lastTick = millis(); }
    bool needsContinuousUpdate() const override { return true; }

    void onButtonSelect() override { _wantsPop = true; }
    void onButtonBack()   override { _wantsPop = true; }
    void onButtonUp()     override { _wantsPop = true; }
    void onButtonDown()   override { _wantsPop = true; }

    bool wantsPop() const override { return _wantsPop; }

    void update() override {
        unsigned long now = millis();
        if (now - _lastTick < 60) return;
        _lastTick = now;
        _t += 0.3f;

        _display.clear();

        // Sample a coarser grid (every 4 pixels) for performance on ESP32
        // This gives a 32x14 plasma that looks great on 128x56
        const int STEP = 4;
        for (int y = 0; y < 56; y += STEP) {
            for (int x = 0; x < 128; x += STEP) {
                float fx = x / 16.0f;
                float fy = y / 16.0f;

                // Sine wave interference
                float v = sin(fx + _t)
                        + sin(fy + _t)
                        + sin(fx + fy + _t)
                        + sin(sqrt(fx * fx + fy * fy + 1.0f) + _t);

                // Threshold: draw pixel if v > 0 (binary plasma on OLED)
                if (v > 0.5f) {
                    _display.drawLine(x, x + STEP - 1, y, y);
                    if (STEP > 2) _display.drawLine(x, x + STEP - 1, y + 1, y + 1);
                }
            }
        }

        _display.setFontSmall();
        _display.drawStr(25, 63, "[Any key to exit]");
        _display.sendBuffer();
    }

private:
    DisplayManager& _display;
    float           _t         = 0.0f;
    unsigned long   _lastTick  = 0;
    bool            _wantsPop  = false;
};
