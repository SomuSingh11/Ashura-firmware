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

// ── Apps ──────────────────────────────────────────────────────
#include "../ui/screens/wled/WledDeviceScreen.h"
#include "../ui/screens/clockApp/ClockFaceScreen.h"

// ── Settings screens ────────────────────────────────────────────
#include "../ui/screens/settings/SystemStatsScreen.h"
#include "../ui/screens/settings/network/NetworkScreen.h"

// ── Vibe system ───────────────────────────────────────────────
#include "../ui/screens/vibes/VibePickerScreen.h"
#include "../ui/screens/vibes/VibePreviewScreen.h"
#include "../ui/screens/vibes/VibePlayerScreen.h"
#include "../application/vibes/VibeRegistry.h"

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

  // ── 6. Wled ─────────────────────────────────────────────────────
  _wled.begin();
  record_create(RECORD_WLED, &_wled);

  // ── 7. Boot UI Sequence ─────────────────────────────────────────
  _bootUI();

  // ── 8. EventBus Wiring ──────────────────────────────────────────
  _wireEvents();

  // ── 10. Button Initialization ────────────────────────────────────
  _initButtons();

  // ── 11. Network & WebSocket Initialization ───────────────────────
  _initNetwork();

  Serial.println("[AshuraCore] ✅ Boot complete");
}


// ============================================================
//  AshuraCore::update  —  Main OS tick (called from loop())
// ============================================================

void AshuraCore::update(){
  unsigned long now = millis();
  
  _updateNetwork(now);

  // ── Companion Systems (Mood Update — mood decay + lerp + blink + micro) ──────────
  _mood.update(now);
  _companion.update(now);

  // ── UI Tick ─────────────────────────────────────────────────
  _ui.update();

  // ── HomeScreen Signals ──────────────────────────────────────
  if (_ui.currentScreen() == _homeScreen) {
    if(_homeScreen->wantsMenu())        _buildAppMenu();
    if(_homeScreen->wantsScreenSaver()) _launchScreenSaver();
  }

  // ── Physical Buttons ────────────────────────────────────────
  bool anyPressed = false;

  _pollButton(PIN_BUTTON_UP,_btnUp,[this]{ _ui.onButtonUp(); }, [this]{ _ui.onLongPressUp();  }, anyPressed);
  _pollButton(PIN_BUTTON_DOWN, _btnDown, [this]{ _ui.onButtonDown(); }, [this]{ _ui.onLongPressDown(); }, anyPressed);
  _pollButton(PIN_BUTTON_SELECT, _btnSelect, [this]{ _ui.onButtonSelect(); }, [this]{ _ui.onLongPressSelect();}, anyPressed);
  _pollButton(PIN_BUTTON_BACK, _btnBack, [this]{ _ui.onButtonBack(); }, [this]{ _ui.onLongPressBack();}, anyPressed);

  // ── Notify mood engine — resets idle timer, wakes from bored/sleepy ───────────────
  if (anyPressed) _mood.onInteraction();
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
                { "TODO: Home Screen", [this]() {
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
          return new WledDeviceScreen(d, _ui, _wled);
        }
    });

  // ── Settings ──────────────────────────────────────────────
  _loader.registerApp({
        "settings", "Settings", "settings",
        [this](DisplayManager& d) -> IScreen* {
            return new SubMenuScreen(d, "Settings", {
                { "Network",      [this]() { _ui.pushScreen(new NetworkScreen(_display, _ui, _wifi, _websocket)); } },
                { "System Stats", [this]() { _ui.pushScreen(new SystemStatsScreen(_display)); } },
            });
        }
    });

  Serial.println(String("[Loader] ") + _loader.apps().size() + " apps registered");
}


// ============================================================
//  _bootUI  —  Read NVS boot pref, push BootScreen
// ============================================================

void AshuraCore::_bootUI()
{
  _homeScreen = new HomeScreen(_display, _companion);
  _ui.pushScreen(_homeScreen);

  int bootIdx = constrain(AshuraPrefs::getBoot(), 0, VIBE_COUNT - 1);
  Serial.println("[Boot] bootIdx = " + String(bootIdx) + " / " + ALL_VIBES[bootIdx].name);

  const Animation* bootAnimation = ALL_VIBES[bootIdx].animation;

  if(bootAnimation == nullptr){
    Serial.println("[Boot] Using SplashScreen");
    _ui.pushScreen(new SplashScreen(_display));
  } else {
    Serial.println("[Boot] Using VibePlayerScreen: " + String(bootAnimation->name));
    _ui.pushScreen(new VibePlayerScreen(_display, bootAnimation));
  }
}


// ============================================================
//  _launchScreenSaver —  Read NVS screensaver pref, push VibePlayerScreen
// ============================================================

void AshuraCore::_launchScreenSaver() {
    int idx = AshuraPrefs::getScreensaver();
    idx     = constrain(idx, 0, VIBE_COUNT - 1);

    const Animation* anim = ALL_VIBES[idx].animation;
    _ui.pushScreen(new VibePlayerScreen(_display, anim));
}



// ============================================================
//  _wireEvents
// ============================================================

// ============================================================
//  _updateNetwork
//
//  Coordinates WiFi + WebSocket with a simple state machine:
//
//  WiFi down  → only run WiFiManager (handles reconnect internally)
//               WebSocket is NOT polled — pointless without WiFi
//
//  WiFi restored → immediately trigger WS connect + NTP sync
//
//  WiFi up    → poll WebSocket normally (handles its own reconnect)
//
//  WiFi lost  → stop polling WebSocket, update home screen badge
// ============================================================

void AshuraCore::_updateNetwork(unsigned long now){
  NetState        prevNet   = _wifi.state();
  WebSocketState  prevWs    = _websocket.webSocketState();

  // ── 1. Always tick WiFiManager ────────────────────────────
    _wifi.update();

  // ── 2. React to WiFi state transitions ────────────────────
  NetState curNet = _wifi.state();

  if(prevNet != NetState::CONNECTED && curNet == NetState::CONNECTED) {
    // WiFi just came up — sync NTP, let WS state machine take over
    Serial.println("[Net] WiFi connected → NTP sync");
    Time().sync();
    _lastNtpSync = now;
  }

  if(prevNet == NetState::CONNECTED && curNet != NetState::CONNECTED) {
    // WiFi just dropped — reset WebSocket cleanly
    Serial.println("[Net] WiFi lost → resetting WebSocket");
    _websocket.resetForWifi(); 
  }

  // ── 3. Tick WebSocket only when WiFi is up ────────────────
  if (curNet == NetState::CONNECTED) {
    _websocket.update();
  }

  // ── 4. NTP re-sync on interval ────────────────────────────
  if (curNet == NetState::CONNECTED &&
    now - _lastNtpSync > NTP_SYNC_INTERVAL &&
    !Time().isSynced()) {
    _lastNtpSync = now;
    Time().sync();
  }

  // ── 5. Always tick TimeManager ────────────────────────────
  Time().update();

  // ── 6. Update badge if either state changed ───────────────
  if (_wifi.state() != _prevNetState || _websocket.webSocketState() != _prevWsState) {
    _prevNetState = _wifi.state();
    _prevWsState  = _websocket.webSocketState();
    _updateBadge();
  }
}


// ============================================================
//  _updateBadge
//
//  Computes compound [WiFi WS] badge from both state machines.
//  Called whenever either state changes — not every tick.
//
//  WiFi symbols:   · idle   ○ connecting/lost   ● connected   ! failed
//  WS symbols:     - idle   ~ connected         * registered  ! failed
// ============================================================

void AshuraCore::_updateBadge() {
  // ── WiFi symbol ───────────────────────────────────────────
  char wSym;
  switch (_wifi.state()) {
        case NetState::CONNECTED:            wSym = '*'; break;  // filled

        case NetState::CONNECTING:
        case NetState::LOST:                 wSym = 'o'; break;  // trying

        case NetState::FAILED:               wSym = '!'; break;  // gave up

        case NetState::IDLE:
        default:                             wSym = '-'; break;  // no creds
    }
  
  // ── WebSocket symbol ──────────────────────────────────────
  char wsSym;
  switch (_websocket.webSocketState()) {
        case WebSocketState::REGISTERED:            wsSym = '*'; break;
        case WebSocketState::CONNECTED:             wsSym = '~'; break;
        case WebSocketState::CONNECTING:            wsSym = 'o'; break;
        case WebSocketState::FAILED:                wsSym = '!'; break;
        case WebSocketState::IDLE:
        default:                             wsSym = '-'; break;
  }

  char badge[8];
  snprintf(badge, sizeof(badge), "[%c %c]", wSym, wsSym);

  if (_homeScreen) _homeScreen->setConnectionStatus(badge);

  Serial.printf("[Badge] %s  WiFi:%d  WS:%d\n",
                  badge,
                  (int)_wifi.state(),
                  (int)_websocket.webSocketState());
}



void AshuraCore::_wireEvents()
{
  // ── WiFi Events ──────────────────────────────────────────────
  Bus().subscribe(AppEvent::WifiConnected, [this](const String& ip) {
    Serial.println("[Core] WiFi ✅ " + ip);
  });
  Bus().subscribe(AppEvent::WifiDisconnected, [this](){
    Serial.println("[Core] WiFi ❌");
  });
  Bus().subscribe(AppEvent::WifiFailed, [this]() {
    Serial.println("[Core] WiFi FAILED");
    // Notification already pushed by WiFiManager
    // MoodEngine → ANNOYED (brief)
  });
  Bus().subscribe(AppEvent::WifiIdle, [this]() {
     Serial.println("[Core] WiFi IDLE — credentials cleared");
  });


  // ── WebSocket → HomeScreen Badge ─────────────────────────────
  Bus().subscribe(AppEvent::WebSocketConnected, [this](){
    Serial.println("[Core] WS connected"); 
  });
  Bus().subscribe(AppEvent::WebSocketDisconnected, [this](){
    Serial.println("[Core] WS disconnected"); 
  });
  Bus().subscribe(AppEvent::WebSocketRegistered, [this](){
    Serial.println("[Core] WS registered ✅");
  });
  Bus().subscribe(AppEvent::WebSocketFailed, [this]() {
    Serial.println("[Core] WS FAILED");
    // Notification already pushed by WebSocketManager
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

  // ── Notification badge dot ────────────────────────────────
  Bus().subscribe(AppEvent::NotificationPushed, [this](const String& title) {
    // HomeScreen reads Notifs().unreadCount() each frame
    // so no explicit call needed — just log here
    Serial.println("[Core] Notification: " + title);
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
  _wifi.init(); // loads NVS creds, starts CONNECTING if found

  // ── Initial Time Sync ───────────────────────────────────────
  if (_wifi.isConnected()){
    Time().sync();
    _lastNtpSync = millis();
  }

  // ── WebSocket Service ───────────────────────────────────────
  record_create(RECORD_WEBSOCKET, &_websocket);
  _websocket.init(); // loads NVS config, wires callbacks | state machine starts in IDLE — update() drives it

  // ── Message Routing Services ────────────────────────────────
  _router.registerService(&_deviceService);

  // ── Seed state tracking ───────────────────────────────────
    _prevNetState = _wifi.state();
    _prevWsState  = _websocket.webSocketState();

    _updateBadge();   // set initial badge on HomeScreen
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

// ============================================================
//  _pollButton  —  Full press/long-press/release handler
//
//  onShortPress fires on RELEASE if held < LONG_PRESS_MS
//  onLongPress  fires ONCE after LONG_PRESS_MS while held
//  anyPressed   set true on any event (for mood interaction)
// ============================================================

void AshuraCore::_pollButton(int pin, BtnState& btn,
                              std::function<void()> onShortPress,
                              std::function<void()> onLongPress,
                              bool& anyPressed)
{
    bool raw = digitalRead(pin);

    // Debounce
    if (raw != btn.lastRaw) btn.lastDebounce = millis();
    btn.lastRaw = raw;
    if ((millis() - btn.lastDebounce) <= BUTTON_DEBOUNCE_MS) return;

    bool isPressed = (raw == LOW);

    if (isPressed && btn.state == HIGH) {
        // Button just went down
        btn.state      = LOW;
        btn.pressStart = millis();
        btn.longFired  = false;

    } else if (isPressed && btn.state == LOW) {
        // Button held — check for long press
        if (!btn.longFired &&
            (millis() - btn.pressStart) >= LONG_PRESS_MS) {
            btn.longFired = true;
            onLongPress();
            anyPressed = true;
        }

    } else if (!isPressed && btn.state == LOW) {
        // Button released
        btn.state = HIGH;
        if (!btn.longFired) {
            // Short press — fire on release
            onShortPress();
            anyPressed = true;
        }
    }
}