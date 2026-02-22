#pragma once
#include <vector>
#include "screens/IScreen.h"
#include "DisplayManager.h"
#include <Arduino.h>
#include "screens/StatusScreen.h"

// ============================================
// UIManager - Owns the screen stack
//
// Navigation:
//   pushScreen(new SomeScreen()) → go forward
//   popScreen()                  → go back (auto-frees memory)
//
// Continuous update:
//   Screens that override needsContinuousUpdate() → true
//   get update() called every loop tick regardless of dirty flag.
//   This powers games and animations.
//
// WantsPop pattern:
//   Animations/splash screens can't call popScreen() themselves.
//   UIManager checks wantsPop() on supported screens each loop.
// ============================================

class UIManager {
    public:
        void init(DisplayManager* display);
        void update();

        void pushScreen(IScreen* screen);
        void popScreen();

        void onButtonUp();
        void onButtonDown();
        void onButtonSelect();
        void onButtonBack();

        IScreen* currentScreen(); // Returns pointer to the current (top) screen.

    private: 
        DisplayManager* _display = nullptr; // Pointer to the display system.
        std::vector<IScreen*> _stack; // stack of screens
};