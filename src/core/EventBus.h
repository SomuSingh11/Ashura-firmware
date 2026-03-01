#pragma once
#include<Arduino.h>
#include<functional>
#include<vector>

// ============================================================
//  AppEvent  —  all system-wide events
// ============================================================
enum class AppEvent {
    // ── WiFi ──────────────────────────────────────────────────
    WifiConnected,
    WifiDisconnected,

    // ── WebSocket ─────────────────────────────────────────────
    WebSocketConnected,
    WebSocketDisconnected,
    WebSocketRegistered,

    // ── Messaging ─────────────────────────────────────────────
    CommandReceived,
    NotificationReceived,
    SendMessage,          // for outgoing WebSocket messages

    // ── Input ─────────────────────────────────────────────────
    ButtonUp,
    ButtonDown,
    ButtonSelect,
    ButtonBack,

    // ── UI ────────────────────────────────────────────────────
    DisplayNeedsUpdate,
    ScreensaverStart,
    ScreensaverStop,

    // ── System ────────────────────────────────────────────────
    SystemTick,   // published every loop(), used by games/animations
    SystemBoot,    // published at the end of init()

    // ── Companion: Pomodoro ───────────────────────────────────
    PomodoroStarted,        // session began     → companion goes FOCUSED
    PomodoroCompleted,      // full session      → companion goes HAPPY
    PomodoroAborted,        // user cancelled    → companion goes IDLE
    PomodoroBreak,          // break started     → companion goes HAPPY
    SpotifyPlaying,         // track is playing  → companion EXCITED
    SpotifyPaused,          // paused / stopped  → companion back to IDLE
};

struct EventSubscription {
    AppEvent event;
    std::function<void(const String& payload)> callback;
};

class EventBus {
    public: 
        static EventBus& instance(){
            static EventBus bus;
            return bus;
        }

    void subscribe(AppEvent event, std::function<void(const String& payload)> cb){
        _subscriptions.push_back({event, cb});
    }

    // subscribe without caring about payload
    void subscribe(AppEvent event, std::function<void()> cb){
         _subscriptions.push_back({ event, [cb](const String&) { cb(); } });
    } 

    void publish(AppEvent event, const String& payload = "") {
    for (auto& sub : _subscriptions) {
      if (sub.event == event) {
        sub.callback(payload);
      }
    }
  }

  private:
    EventBus() = default;
    std::vector<EventSubscription> _subscriptions;
};

// Global shorthand
inline EventBus& Bus() {return EventBus::instance();}
