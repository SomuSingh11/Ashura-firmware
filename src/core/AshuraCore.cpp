#include "config.h"
#include "pins.h"

#include "AshuraRecord.h"
#include "EventBus.h"
#include "TimeManager.h"
#include "core/AshuraCore.h"
#include "storage/AshuraPrefs.h"

#include "../ui/screens/AppMenuScreen.h"
#include "../ui/screens/SubMenuScreen.h"
#include "../ui/screens/bootScreen/SplashScreen.h"

// ── Vibe system ───────────────────────────────────────────────
#include "../ui/screens/vibes/VibePickerScreen.h"
#include "../ui/screens/vibes/VibePreviewScreen.h"
#include "../ui/screens/vibes/VibePlayerScreen.h"
#include "../application/vibes/VibeRegistry.h"

#include "../ui/screens/clockApp/ClockFaceScreen.h"
#include "../ui/screens/screenSaver/PlasmaScreen.h"
#include "../ui/screens/settings/SystemStatsScreen.h"

#include <vector>

// ============================================================
//  AshuraOS::init  —  Full system boot
// ============================================================

void AshuraCore::init()
{
  // ── Debug Serial ─────────────────────────────────────────────
#if DEBUG_SERIAL
  Serial.begin(SERIAL_BAUD);
  delay(300);
  Serial.println("\n================================================");
  Serial.println("        ASHURA OS  —  Booting...");
  Serial.println("================================================");
#endif

  // ── 1. Display Service ──────────────────────────────────────────
  _display.init();
  record_create(RECORD_DISPLAY, &_display);

  // ── 2. GUI / UIManager Service ──────────────────────────────────
  _ui.init(&_display);
  record_create(RECORD_GUI, &_ui);

  // ── 3. Loader Service ───────────────────────────────────────────
  record_create(RECORD_LOADER, &_loader);

  // ── 4. Register Built-in Apps ───────────────────────────────────
  _registerApps();

  // ── 5. Mood Engine & Companion ──────────────────────────────────
  _mood.init();
  _companion.begin();

  // ── 6. Boot UI Sequence ─────────────────────────────────────────
  _bootUI();

  // ── 7. EventBus Wiring ──────────────────────────────────────────
  _wireEvents();

  // ── 8. Button Initialization ────────────────────────────────────
  _initButtons();

  // ── 9. Network & WebSocket Initialization ───────────────────────
  _initNetwork();

  Serial.println("[AshuraCore] ✅ Boot complete");
}

// ============================================================
//  AshuraCore::update  —  Main OS tick (called from loop())
// ============================================================

void AshuraCore::update(){
  unsigned long now = millis();
  
  // ── WiFi Watchdog ────────────────────────────────────────────
  _wifi.update();

  // ── NTP Retry / Time Sync ───────────────────────────────────
  Time().update();
  if (_wifi.isConnected() && !Time().isSynced())
  {
    static unsigned long _lastNtp = 0;
    if (millis() - _lastNtp > NTP_SYNC_INTERVAL)
    {
      _lastNtp = millis();
      Time().sync();
    }
  }

  // ── WebSocket Poll ───────────────────────────────────────────
  _websocket.update();

  // ── Companion Systems (Mood Update — mood decay + lerp + blink + micro) ──────────
  _mood.update(now);
  _companion.update(now);

  // ── UI Tick ─────────────────────────────────────────────────
  _ui.update();

  // ── HomeScreen Signals ──────────────────────────────────────
  if (_ui.currentScreen() == _homeScreen)
  {
    if (_homeScreen->wantsMenu())
      _buildAppMenu();
    if (_homeScreen->wantsScreenSaver())
      _launchScreenSaver();
  }

  // ── Physical Buttons ────────────────────────────────────────
  bool anyPressed = false;

  if (_readButton(PIN_BUTTON_UP, _btnUp))
    _ui.onButtonUp();
  if (_readButton(PIN_BUTTON_DOWN, _btnDown))
    _ui.onButtonDown();
  if (_readButton(PIN_BUTTON_SELECT, _btnSelect))
    _ui.onButtonSelect();
  if (_readButton(PIN_BUTTON_BACK, _btnBack))
    _ui.onButtonBack();

  // ── Notify mood engine — resets idle timer, wakes from bored/sleepy ───────────────
  if (anyPressed)
    _mood.onInteraction();
}

// ============================================================
//  Private — boot helpers
// ============================================================

void AshuraCore::_registerApps()
{
  // ── Clock ─────────────────────────────────────────────────
  _loader.registerApp({
        "clock", "Clock", "clock",
        [this](DisplayManager& d) -> IScreen* {
            return new SubMenuScreen(d, "Clock", {
                { "Clock Face", [this]() { _ui.pushScreen(new ClockFaceScreen(_display)); } },
                { "Pomodoro",   [this]() {  } },
                { "Stopwatch",  [this]() { } },
            });
        }
    });

  // ── Games ─────────────────────────────────────────────────
  _loader.registerApp({
        "games", "Games", "games",
        [this](DisplayManager& d) -> IScreen* {
            return new SubMenuScreen(d, "Games", {
                { "Snake",     [this]() {  } },
            });
        }
    });

  // ── AI Assitant ───────────────────────────────────────────────
  _loader.registerApp({
        "ai", "AI", "ai",
        [this](DisplayManager& d) -> IScreen* {
            return new SubMenuScreen(d, "AI Assistant", {
                { "Chat",     [this]() { /* TODO */ } },
                { "History",  [this]() { /* TODO */ } },
                { "Settings", [this]() { /* TODO */ } },
            });
        }
    });

  // ── Vibes ──────────────────────────────────────────────────
  _loader.registerApp({
        "vibes", "Vibes", "vibes",
        [this](DisplayManager& d) -> IScreen* {
            return new SubMenuScreen(d, "Vibes", {
                { "Screensaver", [this]() {
                    _ui.pushScreen(new VibePickerScreen(
                        _display, _ui, 0, AshuraPrefs::getScreensaver()));
                }},
                { "Boot Screen", [this]() {
                    _ui.pushScreen(new VibePickerScreen(
                        _display, _ui, 1, AshuraPrefs::getBoot()));
                }},
                { "Home Screen", [this]() {
                    _ui.pushScreen(new VibePickerScreen(
                        _display, _ui, 2, AshuraPrefs::getHomeScreen()));
                }},
            });
        }
    });

  // ── Spotify ───────────────────────────────────────────────
  _loader.registerApp({
        "spotify", "Spotify", "spotify",
        [this](DisplayManager& d) -> IScreen* {
            return new SubMenuScreen(d, "Spotify", {
                { "Now Playing", [this]() { /* TODO */ } },
                { "Browse",      [this]() { /* TODO */ } },
                { "Search",      [this]() { /* TODO */ } },
            });
        }
    });

  // ── WLED ──────────────────────────────────────────────────
  _loader.registerApp({
        "wled", "WLED", "wled",
        [this](DisplayManager& d) -> IScreen* {
            return new SubMenuScreen(d, "WLED", {
                { "Devices",    [this]() { /* TODO */ } },
                { "Effects",    [this]() { /* TODO */ } },
                { "Brightness", [this]() { /* TODO */ } },
            });
        }
    });

  // ── Settings ──────────────────────────────────────────────
  _loader.registerApp({
        "settings", "Settings", "settings",
        [this](DisplayManager& d) -> IScreen* {
            return new SubMenuScreen(d, "Settings", {
                { "System Stats", [this]() { _ui.pushScreen(new SystemStatsScreen(_display)); } },
                { "WiFi",         [this]() { /* TODO */ } },
                { "Display",      [this]() { /* TODO */ } },
                { "About",        [this]() { /* TODO */ } },
            });
        }
    });

  Serial.println(String("[Loader] ") + _loader.apps().size() + " apps registered");
}


// ============================================================
//  _bootUI  —  Read NVS boot pref
// ============================================================

void AshuraCore::_bootUI()
{
  _homeScreen = new HomeScreen(_display, _companion);
  _ui.pushScreen(_homeScreen);

  int bootIdx = constrain(AshuraPrefs::getBoot(), 0, VIBE_COUNT - 1);
  _ui.pushScreen(new VibePlayerScreen(_display, ALL_VIBES[bootIdx].animation));
}


// ============================================================
//  _wireEvents
// ============================================================

void AshuraCore::_wireEvents()
{
  // ── WiFi Events ──────────────────────────────────────────────
  Bus().subscribe(AppEvent::WifiConnected, [this](const String& ip) {
    Serial.println("[Core] WiFi ✅ " + ip);
  });
  Bus().subscribe(AppEvent::WifiDisconnected, [this](){
    Serial.println("[Core] WiFi ❌");
    if (_homeScreen) _homeScreen->setConnectionStatus("[ - ]"); 
  });

  // ── WebSocket → HomeScreen Badge ─────────────────────────────
  Bus().subscribe(AppEvent::WebSocketConnected, [this](){
    if (_homeScreen) _homeScreen->setConnectionStatus("[ ~ ]"); 
  });
  Bus().subscribe(AppEvent::WebSocketDisconnected, [this](){
    _wsStatus = "Offline";
    if (_homeScreen) _homeScreen->setConnectionStatus("[ - ]"); 
  });
  Bus().subscribe(AppEvent::WebSocketRegistered, [this](){
    _wsStatus = "Active";
    if (_homeScreen) _homeScreen->setConnectionStatus("[ * ]"); 
  });


  // ── Incoming Commands → Router ───────────────────────────────
  Bus().subscribe(AppEvent::CommandReceived, [this](const String& json) {
    _router.route(json);
  });

  // ── Notifications → HomeScreen Ticker ───────────────────────
  // (MoodEngine also subscribed → SURPRISED flash)
  Bus().subscribe(AppEvent::NotificationReceived, [this](const String& payload){
    if (_homeScreen) _homeScreen->setLastMessage(payload); 
  });

  // ── Outgoing Messages → WebSocket ───────────────────────────
  Bus().subscribe(AppEvent::SendMessage, [this](const String& json){ 
  _websocket.send(json);
  });
}


// ============================================================
//  _initButtons
// ============================================================

void AshuraCore::_initButtons()
{
  pinMode(PIN_BUTTON_UP, INPUT_PULLUP);
  pinMode(PIN_BUTTON_DOWN, INPUT_PULLUP);
  pinMode(PIN_BUTTON_SELECT, INPUT_PULLUP);
  pinMode(PIN_BUTTON_BACK, INPUT_PULLUP);
}


// ============================================================
//  _initNetwork
// ============================================================

void AshuraCore::_initNetwork()
{
  // ── WiFi Service ─────────────────────────────────────────────
  record_create(RECORD_WIFI, &_wifi);
  // WiFi (blocking connect with timeout in WiFiManager::init)
  _wifi.init(WIFI_SSID, WIFI_PASSWORD);

  // ── Initial Time Sync ───────────────────────────────────────
  if (_wifi.isConnected()) Time().sync();

  // ── WebSocket Service ───────────────────────────────────────
  record_create(RECORD_WEBSOCKET, &_websocket);
  _websocket.init();

  // ── Message Routing Services ────────────────────────────────
  _router.registerService(&_deviceService);

  // ── WebSocket Connection ────────────────────────────────────
  // Non-blocking after this call
  _websocket.connect();
}


// ============================================================
//  Private — runtime helpers
// ============================================================

// ============================================================
//  _buildAppMenu
// ============================================================

void AshuraCore::_buildAppMenu()
{
  // Build item list from Loader registry
  std::vector<AppItem> items;

  for (auto& [id, desc] : _loader.apps())
  {
    AppItem item;
    item.icon = desc.icon;
    item.name = desc.name;

    // Capture id by value for lambda
    String capturedId = id;
    item.onLaunch = [this, capturedId]()
      {
        IScreen* s = _loader.buildApp(capturedId, _display);
        if (s)
          _ui.pushScreen(s);
      };
    items.push_back(std::move(item));
  }
  _ui.pushScreen(new AppMenuScreen(_display, items));
}


// ============================================================
//  _launchScreenSaver
// ============================================================

void AshuraCore::_launchScreenSaver() {
    int idx = AshuraPrefs::getScreensaver();
    idx     = constrain(idx, 0, VIBE_COUNT - 1);

    const Animation* anim = ALL_VIBES[idx].animation;
    _ui.pushScreen(new VibePlayerScreen(_display, anim));
}


// ============================================================
//  _readButton  —  debounced, true once on falling edge
// ============================================================

bool AshuraCore::_readButton(int pin, BtnState& btn)
{
  bool raw = digitalRead(pin);
  if (raw != btn.lastRaw)
    btn.lastDebounce = millis();
  btn.lastRaw = raw;

  if ((millis() - btn.lastDebounce) > BUTTON_DEBOUNCE_MS)
  {
    if (raw != btn.state)
    {
      btn.state = raw;
      if (btn.state == LOW)
        return true;
    }
  }
  return false;
}
