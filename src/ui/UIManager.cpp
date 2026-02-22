#include "UIManager.h"
#include "screens/StatusScreen.h"
#include "screens/bootScreen/SplashScreen.h"

// 1. init
void UIManager::init(DisplayManager* display){
    _display = display;
}

void UIManager::update(){
    IScreen* screen = currentScreen();
    if(!screen) return;

    // Continuous update screens ignore dirty flag (for animations/games)
    if(screen->needsContinuousUpdate()){
        screen->update();

        if(screen->wantsPop()){
            popScreen();
            return;
        }
    } else {
        // Normal dirty-flag update
        if(screen->isDirty()){
            screen->update();
        }
    }
}

// 2. Navigation Logic
void UIManager::pushScreen(IScreen* screen){
    if(currentScreen()) currentScreen()->onExit();
    _stack.push_back(screen);
    screen->onEnter();
    screen->markDirty();
}

void UIManager::popScreen(){
    if(_stack.size() <= 1) return;
    currentScreen()->onExit();
    delete _stack.back();       // Freeing the heap memory (object)
    _stack.pop_back();          // Removes pointer from container
    currentScreen()->onEnter();
    currentScreen()->markDirty();
}

IScreen* UIManager::currentScreen(){
    if(_stack.empty()) return nullptr;
    return _stack.back();
}

// 3. Button Forwarding
void UIManager::onButtonUp(){
    if(currentScreen()) currentScreen()->onButtonUp();
}
void UIManager::onButtonDown(){
    if(currentScreen()) currentScreen()->onButtonDown();
}
void UIManager::onButtonSelect(){
    if(currentScreen()) currentScreen()->onButtonSelect();
}
void UIManager::onButtonBack(){
    if(currentScreen()) currentScreen()->onButtonBack();
    popScreen();
}