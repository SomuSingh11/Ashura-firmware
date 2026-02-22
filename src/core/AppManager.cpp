#include "AppManager.h"
#include "EventBus.h"
#include "config.h"
#include "pins.h"

#include <vector>

#include "../ui/screens/StatusScreen.h"
#include "../ui/screens/bootScreen/SplashScreen.h"
#include "../ui/screens/AppMenuScreen.h"
#include "../ui/screens/clockApp/ClockApp.h"
#include "../ui/screens/gamesApp/GamesApp.h"
#include "../ui/screens/screenSaver/PlasmaScreen.h"

void AppManager::init() {
#if DEBUG_SERIAL
  Serial.begin(SERIAL_BAUD);
  Serial.println("\n================================");
  Serial.println("  ASHURA Core  Booting...");
  Serial.println("================================");
#endif

  // ===== 1. Init Display first =====
  _display.init();

  // ===== 2. Initialize UI with display pointer =====
  _ui.init(&_display);

  // ===== 3. Boot Sequence [Splash -> Home] =====
  _homeScreen = new HomeScreen(_display);

  _ui.pushScreen(_homeScreen); //  HomeScreen stays at bottom of stack forever
  _ui.pushScreen(new SplashScreen(_display)); // SplashScreen auto-pops itself via wantsPop()

  // ===== 4. Wire all cross-module events =====
  _wireEvents();

  // ===== 5.Button Pins =====
  pinMode(PIN_BUTTON_UP, INPUT_PULLUP);
  pinMode(PIN_BUTTON_DOWN, INPUT_PULLUP);
  pinMode(PIN_BUTTON_SELECT, INPUT_PULLUP);
  pinMode(PIN_BUTTON_BACK, INPUT_PULLUP);

  // ===== 6. Connect WiFi =====
  _wifi.init(WIFI_SSID, WIFI_PASSWORD);

  // ===== 7. Init WebSocket =====
  _websocket.init();

  // ===== 8. Register Service =====
  _router.registerService(&_deviceService);

  // ===== 9. Connect WebSocket =====
  _websocket.connect();

  Serial.println("✅ ASHURA OS ready!");
}

void AppManager::update() {
  // ===== Websocket update =====
  _websocket.update();

  // ===== UI tick =====
  _ui.update();

  // ===== HomeScreen Signals =====
  if(_homeScreen) {
    if (_homeScreen->wantsMenu()){
      _buildAppMenu();
    }

    if(_homeScreen->wantsScreenSaver()){
      _lauchScreenSaver();
    }
  }

  // ===== Physical Buttons =====
  if(_readButton(PIN_BUTTON_UP, _btnUp)) _ui.onButtonUp();
  if(_readButton(PIN_BUTTON_DOWN, _btnDown)) _ui.onButtonDown();
  if(_readButton(PIN_BUTTON_SELECT, _btnSelect)) _ui.onButtonSelect();
  if(_readButton(PIN_BUTTON_BACK  , _btnBack)) _ui.onButtonBack();
}

// ============================================
// _wireEvents - All cross-module subscriptions
// ============================================
void AppManager::_wireEvents(){

  // WiFi -> display status
  Bus().subscribe(AppEvent::WifiConnected, [this](const String& ip) {
    //_display.showBootStatus("WiFi Ok", ip.c_str()); // TODO: Replace this with Message Screen
    //delay(1000);
  });

  Bus().subscribe(AppEvent::WifiDisconnected, [this](){
    //_display.showBootStatus("WiFi Failed", "Offline Mode"); // TODO: Replace this with Message Screen
    //delay(1000);
  });

  // WebSocket Connection State -> Update HomeScreen Badge
  Bus().subscribe(AppEvent::WebSocketConnected, [this]() {
    _websocketStatus = "Connecting";
    if(_homeScreen) _homeScreen->setConnectionStatus("[ ~ ]");
  });

  Bus().subscribe(AppEvent::WebSocketDisconnected, [this]() {
    _websocketStatus = "Offline";
    if (_homeScreen) _homeScreen->setConnectionStatus("[ - ]");
  });

  Bus().subscribe(AppEvent::WebSocketRegistered, [this]() {
    _websocketStatus = "Active";
    _websocketConnectedAt = millis();
    if (_homeScreen) _homeScreen->setConnectionStatus("[ * ]");
  });

  Bus().subscribe(AppEvent::CommandReceived, [this](const String& json){
    _router.route(json);
  });

  Bus().subscribe(AppEvent::NotificationReceived, [this](const String& payload){
    if(_homeScreen) _homeScreen->setLastMessage(payload);
  });
}

// ============================================
// _buildAppMenu - Constructs and pushes the CORE app launcher
// ============================================
void AppManager::_buildAppMenu(){

  std::vector<AppItem> apps = {
    {
      ">_", "Clock", 
      [this](){
        _ui.pushScreen(new ClockApp(_display,[this](IScreen* s){ _ui.pushScreen(s); }));
      }
    },
    {
      ">>", "Games",
      [this](){
        _ui.pushScreen(new GamesApp(_display, [this](IScreen* s){ _ui.pushScreen(s); }));
      }
    }
  };

  _ui.pushScreen(new AppMenuScreen(_display, apps));
}

// ============================================
// _launchScreensaver - Random animation
// ============================================
void AppManager::_lauchScreenSaver(){
  // int rand = random(0, 3);
  // switch(rand){
  //   case 0: _ui.pushScreen(new PlasmaScreen(_display));     break;
  //   case 1: _ui.pushScreen(new MatrixRainScreen(_display)); break;
  //   case 2: _ui.pushScreen(new StarfieldScreen(_display));  break;
    // _ui.pushScreen(new PlasmaScreen(_display));
    _ui.pushScreen(new PlasmaScreen(_display));
}


// ============================================
// _readButton - Debounced button read
// Returns true once on falling edge (press)
// ============================================
bool AppManager::_readButton(int pin, BtnState& btn){
  bool raw = digitalRead(pin);

  if(raw != btn.lastRaw){
    btn.lastDebounce = millis();
  }
  btn.lastRaw = raw;

  if((millis() - btn.lastDebounce) > BUTTON_DEBOUNCE_MS){
    if(raw != btn.state){
      btn.state = raw;
      if(btn.state == LOW){
        return true;
      }
    }
  }
  return false;
}
