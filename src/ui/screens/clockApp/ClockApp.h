#pragma once
#include "config.h"
#include "../IScreen.h"
#include "../../DisplayManager.h"
#include "ClockFaceScreen.h"
#include <vector>
#include <functional>
#include <Arduino.h>


class ClockApp : public IScreen {
    public:
        ClockApp(DisplayManager& display, std::function<void(IScreen*)> pushFn) :
            _display(display), _push(pushFn){}
        
        void onEnter() override {
            _cursor = 0;
            _dirty = true;
        }

        void onButtonDown() override {
            _cursor = (_cursor+1) % CLOCKAPP_LIST_SIZE;
            _dirty = true;
        }

        void onButtonUp() override {
            _cursor = (_cursor-1 + CLOCKAPP_LIST_SIZE) % CLOCKAPP_LIST_SIZE;
            _dirty = true;
        }

        // onButtonBack is handled by UI_Manager (pop screen)

        void onButtonSelect() override {
            switch(_cursor){
                case 0: _push(new ClockFaceScreen(_display)); break;
                // case 1: ;
                // case 2: ;
            }
        }

        void update() override {
            _display.clear();

            _display.setFontLarge();
            _display.drawStr(0, 10, "Clock");
            _display.drawLine(0, 128, 12, 12);

            const char* items[] = {"Clock Face", "Stopwatch", "Timer"};
            _display.setFontMedium();

            for(int i=0; i<CLOCKAPP_LIST_SIZE; i++){
                int y = 26+i*13;

                if(i == _cursor){
                    _display.drawStr(0, y, ">");
                    _display.drawStr(10, y, items[i]);
                } else {
                    _display.drawStr(10, y, items[i]);
                }
            }

            _display.setFontSmall();
            _display.drawStr(0, 63, "Back=Apps");

            _display.sendBuffer();
            _dirty = false;
        }


    private:
        DisplayManager&                 _display;
        std::function<void(IScreen*)>   _push;
        int                             _cursor = 0;
};