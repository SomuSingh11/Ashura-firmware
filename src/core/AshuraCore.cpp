#include "config.h"
#include "pins.h"

#include "AshuraRecord.h"
#include "EventBus.h"
#include "TimeManager.h"
#include "core/AshuraCore.h"

#include "../ui/screens/SubMenuScreen.h"
#include "../ui/screens/bootScreen/SplashScreen.h"
#include "../ui/screens/clockApp/ClockFaceScreen.h"
#include "../ui/screens/AppMenuScreen.h"
#include "../ui/screens/screenSaver/PlasmaScreen.h"

#include <vector>


// ============================================================
//  AshuraOS::init  —  Full system boot
// ============================================================

void AshuraCore::init() {
  // ===== 1. Debug serial =====
  #if DEBUG_SERIAL
      Serial.begin(SERIAL_BAUD);
      delay(300);
      Serial.println("\n================================================");
      Serial.println("        ASHURA OS  —  Booting...");
      Serial.println("================================================");
  #endif

  // ===== 2. Display Service =====
  _display.init();
  record_create(RECORD_DISPLAY, &_display);

  // ===== 3. GUI / UIManager Service =====
  _ui.init(&_display);
  record_create(RECORD_GUI, &_ui);

  // ===== 4. Loader Service =====
  record_create(RECORD_LOADER, &_loader);

  // ===== 5. Register all buil-in apps =====
  _registerApps();

  // ===== 6. Boot UI Sequence =====
  _bootUI();

  // ===== 7. EventBus Wiring =====
  _wireEvents();

  // ===== 8. Button Initialization =====
  _initButtons();

  // ===== 9. Network + Websocket Initialization =====
  _initNetwork();

  Serial.println("[AshuraCore] ✅ Boot complete");
}

// ============================================================
//  AshuraOS::update  —  Main OS tick (called from loop())
// ============================================================

void AshuraCore::update() {
  // ===== WiFi Watchdog =====
  _wifi.update();

  // ===== NTP retry =====
  Time().update();
  if(_wifi.isConnected() && !Time().isSynced()) {
    static unsigned long _lastNtp = 0;
    if(millis() - _lastNtp > NTP_SYNC_INTERVAL) {
      _lastNtp = millis();
      Time().sync();
    }
  }

  // ===== Websocket poll =====
  _websocket.update();

  // ===== UI Tick =====
  _ui.update();

  // ===== HomeScreen Signals =====
  if(_ui.currentScreen() == _homeScreen){
    if(_homeScreen->wantsMenu()) _buildAppMenu();
    if(_homeScreen->wantsScreenSaver()) _launchScreenSaver();
  }

  // ===== Physical Buttons =====
  if (_readButton(PIN_BUTTON_UP,     _btnUp))     _ui.onButtonUp();
  if (_readButton(PIN_BUTTON_DOWN,   _btnDown))   _ui.onButtonDown();
  if (_readButton(PIN_BUTTON_SELECT, _btnSelect)) _ui.onButtonSelect();
  if (_readButton(PIN_BUTTON_BACK,   _btnBack))   _ui.onButtonBack();
}

// ============================================================
//  Private — boot helpers
// ============================================================

void AshuraCore::_registerApps() {
  // ── Clock ─────────────────────────────────────────────────
  _loader.registerApp({
    "clock","Clock", "clock",
    [this](DisplayManager& d) -> IScreen*{
      return new SubMenuScreen(d, "Clock", {
        {"Clock Face", [this, &d]() { _ui.pushScreen(new ClockFaceScreen(d)); }},
        {"Stopwatch", [this, &d]() { _ui.pushScreen(new ClockFaceScreen(d)); }},
        {"Timer", [this, &d]() { _ui.pushScreen(new ClockFaceScreen(d)); }},
        {"Pomodore", [this, &d]() { _ui.pushScreen(new ClockFaceScreen(d)); }},
      });
    }
  });

  // ── Games ─────────────────────────────────────────────────
  _loader.registerApp({
    "games", "Games", "games", 
    [this](DisplayManager& d) -> IScreen*{
      return new SubMenuScreen(d, "Games", {
        {"Snake", [this, &d]() { /* _ui.pushScreen(new SnakeGame(d)); */ }},
        {"Pong vs AI", [this, &d]() { /* _ui.pushScreen(new PongGame(d)); */ }},
      });
    }
  });

  // ── Spotify ───────────────────────────────────────────────
  _loader.registerApp({
    "spotify", "Spotify", "spotify",
    [this](DisplayManager& d) -> IScreen*{
      return new SubMenuScreen(d, "Spotify", {
        {"Connect to Spotify", [this, &d]() { /* _ui.pushScreen(new SpotifyScreen(d)); */ }},
        {"Now Playing", [this, &d]() { /* _ui.pushScreen(new SpotifyScreen(d)); */ }},
        {"Browse", [this, &d]() { /* _ui.pushScreen(new SpotifyScreen(d)); */ }},
        {"Search", [this, &d]() { /* _ui.pushScreen(new SpotifyScreen(d)); */ }},
      });
    }
  });

  // ── WLED ──────────────────────────────────────────────────
  _loader.registerApp({
    "wled", "WLED Controller", "wled",
    [this](DisplayManager& d) -> IScreen*{
      return new SubMenuScreen(d, "WLED Controller", {
        {"Scan", [this, &d]() { /* _ui.pushScreen(new WLEDScreen(d)); */ }},
        {"Connect to WLED", [this, &d]() { /* _ui.pushScreen(new WLEDScreen(d)); */ }},
        {"Presets", [this, &d]() { /* _ui.pushScreen(new WLEDScreen(d)); */ }},
        {"Color Picker", [this, &d]() { /* _ui.pushScreen(new WLEDScreen(d)); */ }},
        {"Effects", [this, &d]() { /* _ui.pushScreen(new WLEDScreen(d)); */ }},
      });
    }
  });

  // ── Settings ──────────────────────────────────────────────
  _loader.registerApp({
    "settings", "Settings", "settings",
    [this](DisplayManager& d) -> IScreen*{
      return new SubMenuScreen(d, "Settings", {
        {"WiFi", [this, &d]() { /* _ui.pushScreen(new WiFiSettingsScreen(d)); */ }},
        {"WebSocket", [this, &d]() { /* _ui.pushScreen(new WebSocketSettingsScreen(d)); */ }},
        {"Device Info", [this, &d]() { /* _ui.pushScreen(new DeviceInfoScreen(d)); */ }},
        {"System Logs", [this, &d]() { /* _ui.pushScreen(new SystemLogsScreen(d)); */ }},
      });
    }
  });

  Serial.println(String("[Loader] ") + _loader.apps().size() + " apps registered");
}

void AshuraCore::_bootUI() {
  _homeScreen = new HomeScreen(_display);
  _ui.pushScreen(_homeScreen);
  _ui.pushScreen(new SplashScreen(_display));
}

void AshuraCore::_wireEvents() {
  // ===== WiFi Events =====
  Bus().subscribe(AppEvent::WifiConnected, [this](const String& ip){
    Serial.println("[Core] WiFi ✅ " + ip);
  });
  Bus().subscribe(AppEvent::WifiConnected, [this](){
    Serial.println("[Core] WiFi ❌");
    if (_homeScreen) _homeScreen->setConnectionStatus("[ - ]");
  });

  // ===== Websocket state -> HomeScreen Badge =====
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

  // ===== Incoming Messages -> MessageRouter =====
  Bus().subscribe(AppEvent::CommandReceived, [this](const String& json){
    _router.route(json);
  });

  // ===== Notifications -> HomeScreen Ticker =====
  Bus().subscribe(AppEvent::NotificationReceived, [this](const String& payload){
    if (_homeScreen) _homeScreen->setLastMessage(payload);
  });

  // ===== Outgoing Messages -> WebSocket =====
  Bus().subscribe(AppEvent::SendMessage, [this](const String& json){
    _websocket.send(json);
  });
}

void AshuraCore::_initButtons() {
  pinMode(PIN_BUTTON_UP,     INPUT_PULLUP);
  pinMode(PIN_BUTTON_DOWN,   INPUT_PULLUP);
  pinMode(PIN_BUTTON_SELECT, INPUT_PULLUP);
  pinMode(PIN_BUTTON_BACK,   INPUT_PULLUP);
}

void AshuraCore::_initNetwork(){
  // WiFi (blocking connect with timeout in WiFiManager::init)
  record_create(RECORD_WIFI, &_wifi);
  _wifi.init(WIFI_SSID, WIFI_PASSWORD);

  if (_wifi.isConnected()) {
    Time().sync();
  }

  // WebSocket
  record_create(RECORD_WEBSOCKET, &_websocket);
  _websocket.init();

  // Message routing services
  _router.registerService(&_deviceService);

  // Connect WS (non-blocking after this)
  _websocket.connect();
}

// ============================================================
//  Private — runtime helpers
// ============================================================

void AshuraCore::_buildAppMenu() {
  // Build item list from Loader registry
  std::vector<AppItem> items;

  for(auto& [id, desc] : _loader.apps()){
    AppItem item;
    item.icon = desc.icon;
    item.name = desc.name;

    // Capture id by value for lambda
    String capturedId = id;
    item.onLaunch = [this, capturedId](){
      IScreen* s = _loader.buildApp(capturedId, _display);
      if (s) _ui.pushScreen(s);
    };
    items.push_back(std::move(item));
  }
  _ui.pushScreen(new AppMenuScreen(_display, items));
}

void AshuraCore::_launchScreenSaver() {
  _ui.pushScreen(new PlasmaScreen(_display));
}

bool AshuraCore::_readButton(int pin, BtnState& btn) {
  bool raw = digitalRead(pin);
  if (raw != btn.lastRaw) btn.lastDebounce = millis();
  btn.lastRaw = raw;

  if ((millis() - btn.lastDebounce) > BUTTON_DEBOUNCE_MS) {
    if (raw != btn.state) {
      btn.state = raw;
      if (btn.state == LOW) return true;
    }
  }
  return false;
}
