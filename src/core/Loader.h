#pragma once
#include <Arduino.h>
#include <map>
#include <functional>
#include "../ui/screens/IScreen.h"
#include "../core/DisplayManager.h"

// ============================================================
// Loader
// ============================================================
//
// Purpose:
// --------
// Acts as an application registry and builder.
//
// Responsibilities:
// -----------------
// ✔ Keeps track of all available applications
// ✔ Stores app metadata (id, name, icon)
// ✔ Creates application screens on demand
//
// How it works:
// -------------
// ✔ Applications register themselves at boot: loader.registerApp({...});
// ✔ Later, any part of the firmware can request: loader.buildApp("clock");
//
// Loader then calls the app’s factory function
// and returns a newly created IScreen*.
//
// IMPORTANT:
// ----------
// Loader ONLY builds screens.
// It does NOT:
//  ❌ Manage navigation
//  ❌ Switch views
//  ❌ Push scenes
//
// Those responsibilities belong to:
// ✔ SceneManager
// ✔ ViewDispatcher
//
// Example Usage:
// --------------
//
// Registering an app:
//
//   loader.registerApp({
//       "clock",
//       "Clock",
//       "CL",
//       createClockScreen
//   });
//
// Launching an app:
//
//   IScreen* screen = loader.buildApp("clock", display);
//   if(screen) {
//       sceneManager.nextScene(Scene::Clock);
//       viewDispatcher.switchTo(View::Clock);
//   }
//
// ============================================================

using AppFactory = std::function<IScreen*(DisplayManager&)>;

struct AppDescriptor {
    String      appid;
    String      name;
    String      icon;
    AppFactory factory;
};

class Loader {
    public:
    // ========================================================
    // App Registration
    // ========================================================

    void registerApp(AppDescriptor descriptor){
        _apps[descriptor.appid] = descriptor;
    }

    // ========================================================
    // App Launching
    // ========================================================

    // Returns a new IScreen* for the caller to push; nullptr if unknown
    IScreen* buildApp(const String& appid, DisplayManager& display){
        auto it = _apps.find(appid);
        if(it == _apps.end()) return nullptr;
        return it->second.factory(display);
    }

    // ========================================================
    // Listing
    // ========================================================
    const std::map<String, AppDescriptor>& apps() const { return _apps; }

    bool appExists(const String& appid) const {
        return _apps.find(appid) != _apps.end();
    }

    private:
        std::map<String, AppDescriptor> _apps;
};

