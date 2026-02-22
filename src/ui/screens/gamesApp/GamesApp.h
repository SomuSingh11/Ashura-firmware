#pragma once
#include "config.h"
#include "../IScreen.h"
#include "../../DisplayManager.h"

class GamesApp : public IScreen {
    public:
        GamesApp(DisplayManager& display, std::function<void(IScreen*)> pushFn)
            : _display(display), _push(pushFn){}

        void onEnter() override {
            _cursor = 0;
            _dirty = true;
        }

        void onButtonDown() override {
            _cursor = (_cursor+1) % GAMESAPP_LIST_SIZE;
            _dirty = true; 
        }

        void onButtonUp() override {
            _cursor = (_cursor-1+GAMESAPP_LIST_SIZE) % GAMESAPP_LIST_SIZE;
            _dirty = true;
        }

        void onButtonSelect() override {
            switch(_cursor) {
                // case 0: _push(new SnakeGame(_display)); break;
                // case 1: _push(new PongGame(_display));  break;
            }
        }

        void update() override {
            _display.clear();

            _display.setFontLarge();
            _display.drawStr(0, 10, "Games");
            _display.drawLine(0, 128, 12, 12);

            const char* items[] = {"Snake", "Pong vs AI"};
            const char* hints[] = {
                "Classic snake",
                "Beat the AI"
            };

            _display.setFontMedium();

            for(int i=0; i<GAMESAPP_LIST_SIZE; i++){
                int y = 28 + i * 16;
                if(i == _cursor){
                    _display.drawStr(0, y, ">");
                    _display.drawStr(10, y, items[i]);
                    _display.setFontSmall();
                    _display.drawStr(10, y+9, hints[i]);
                    _display.setFontMedium();
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