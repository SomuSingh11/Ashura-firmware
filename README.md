# Ashura Firmware

A custom ESP32-based OS firmware for a smart desk companion device. Ashura combines a real-time OLED UI, WebSocket connectivity, WLED LED control, an animated companion system, and a modular app launcher — all running on an ESP32 microcontroller.

---

## Features

- **Animated Companion** — Expressive eye-based companion with mood states (idle, happy, bored, sleepy, focused, surprised, annoyed, excited), smooth lerp transitions, blinking, and micro-behaviours (random glances, wide eyes)
- **App Launcher** — Flipper Zero-style icon menu with scrollbar, XBM icons, and push/pop navigation
- **WLED Integration** — Full HTTP client for WLED LED control: power, brightness, effects, colors, speed/intensity sliders, and mDNS device discovery
- **WebSocket Client** — Persistent WS connection with auto-reconnect, heartbeat, device registration, and message routing to services
- **Vibe System** — PROGMEM XBM animation player (screensaver, boot screen), with a picker/preview UI and NVS persistence
- **Clock App** — Analog + digital clock face with NTP sync via `TimeManager`
- **System Stats Screen** — Live scrollable readout of uptime, heap, WiFi signal, IP, MAC, CPU freq, flash size, chip info, SDK version
- **Screensaver** — Auto-launches after configurable idle timeout; any button press exits
- **Persistent Preferences** — NVS-backed user preferences for screensaver, boot, and home screen vibe selection
- **EventBus** — Decoupled pub/sub system wiring WiFi, WebSocket, notifications and other events to UI and companion mood
- **Message Router** — Chain-of-responsibility pattern routing incoming WS JSON to registered services

---

## Hardware

| Component | Details |
|-----------|---------|
| MCU | ESP32 (tested on ESP32-S3) |
| Display | SSD1306 128×64 OLED (I²C) |
| Buttons | 4× tactile buttons (Up, Down, Select, Back) with INPUT_PULLUP |

### Pin Configuration

Pins are defined in `src/pins.h`. Default I²C pins for OLED are set via `OLED_SDA` / `OLED_SCL`. Button pins are `PIN_BUTTON_UP`, `PIN_BUTTON_DOWN`, `PIN_BUTTON_SELECT`, `PIN_BUTTON_BACK`.

---

## Project Structure

```
src/
├── main.cpp                        # Entry point (setup/loop → AshuraCore)
├── core/
│   ├── AshuraCore.h / .cpp         # Top-level OS kernel: boot, update loop, wiring
│   ├── AshuraRecord.h              # Global service registry (record_create / record_get)
│   ├── DisplayManager.h            # U8G2 OLED wrapper
│   ├── EventBus.h                  # Typed pub/sub event system
│   ├── Loader.h                    # App registry & factory
│   ├── SceneManager.h              # Stack-based scene navigation
│   ├── TimeManager.h               # NTP time sync singleton
│   ├── UIManager.h                 # Screen stack + push/pop navigation
│   ├── ViewDispatcher.h            # Active screen input routing
│   └── WiFiManager.h / .cpp        # WiFi connect/reconnect with EventBus events
├── network/
│   ├── WebSocketManager.h / .cpp   # WS client: connect, heartbeat, message handling
│   ├── MessageRouter.h / .cpp      # Routes JSON messages to registered services
├── services/
│   ├── IService.h                  # Service interface
│   ├── DeviceService.h / .cpp      # Handles display_message commands & notifications
│   └── WledService.h               # WLED WS command handler (stub, extensible)
├── companion/
│   ├── CompanionRenderer.h         # Draws animated eyes anywhere on screen
│   ├── MoodEngine.h                # Mood state machine driven by EventBus events
│   └── Eyeparams.h                 # EyeParams struct + named mood presets
├── application/
│   ├── vibes/
│   │   ├── Animation.h             # PROGMEM animation descriptor struct
│   │   ├── AnimationPlayer.h       # Frame player (Mode 1: autonomous, Mode 2: composited)
│   │   ├── VibeRegistry.h          # Master list of all vibe animations
│   │   └── gifs/                   # Individual XBM animation headers
│   └── wled/
│       ├── WledClient.h            # HTTP GET/POST client for WLED API
│       ├── WledDevice.h            # Device struct (name, IP, port, URL helpers)
│       ├── WledDiscovery.h         # mDNS scan for _wled._tcp devices
│       ├── WledManager.h / .cpp    # Device registry, NVS persistence, active session
│       └── WledState.h             # State struct + partial JSON builders
├── storage/
│   └── AshuraPrefs.h               # NVS preferences (screensaver, boot, home screen)
└── ui/
    └── screens/
        ├── IScreen.h               # Screen interface (lifecycle, input, dirty flag)
        ├── AppMenuScreen.h         # 3-row icon menu with scrollbar
        ├── SubMenuScreen.h         # 4-item scrollable submenu
        ├── HomeScreen.h            # Desktop: companion eyes + clock + status
        ├── bootScreen/
        │   └── SplashScreen.h      # Boot animation with typewriter effect
        ├── clockApp/
        │   └── ClockFaceScreen.h   # Analog + digital clock face
        ├── settings/
        │   └── SystemStatsScreen.h # Live system diagnostics
        ├── vibes/
        │   ├── VibePickerScreen.h  # Animation list picker
        │   ├── VibePreviewScreen.h # Fullscreen preview + confirm/save
        │   └── VibePlayerScreen.h  # Fullscreen animation loop (screensaver)
        └── wled/
            ├── WledDeviceScreen.h  # Device list + scan
            ├── WledMainScreen.h    # Per-device control menu
            ├── WledPowerScreen.h   # Power toggle
            ├── WledBrightnessScreen.h  # Brightness slider
            ├── WledEffectsScreen.h # Scrollable effects list with live preview
            ├── WledColorScreen.h   # Color presets + hue scrubber
            └── WledSpeedScreen.h   # Speed & intensity dual sliders
```

## Navigation Model

Ashura uses a simple push/pop screen stack managed by `UIManager`:

- **SELECT / DOWN** on HomeScreen → opens `AppMenuScreen`
- **SELECT** on a menu item → pushes the app screen
- **BACK** → pops current screen (handled automatically by `UIManager::onButtonBack`)
- Screens with `needsContinuousUpdate() = true` are redrawn every loop tick (animations, games)
- Screens signal self-close via `wantsPop() = true` (splash screens, animations)

Long-press on any button fires `onLongPressX()` — currently used for WLED device scan (long-press UP on `WledDeviceScreen`).

---

## Adding a New App

1. Register in `AshuraCore::_registerApps()`:

```cpp
_loader.registerApp({
    "myapp", "My App", "myapp",
    [this](DisplayManager& d) -> IScreen* {
        return new MyAppScreen(d, _ui);
    }
});
```

2. Add an XBM icon in `AppMenuScreen.h` and wire it in `_appIcon()`.

3. Optionally handle incoming WebSocket commands by implementing `IService` and registering with `_router.registerService(&myService)`.

---

## Adding a New Vibe Animation

1. Convert your GIF to XBM frames and place the header in `src/application/vibes/gifs/`.
2. Include it in `VibeRegistry.h` and append to `ALL_VIBES[]` (never reorder — NVS stores the index).

---

## WebSocket Protocol

All messages are JSON. The device sends:

```json
{ "type": "register", "deviceId": "ashura-01", "deviceType": "esp32", "data": { "name": "Ashura", "ip": "...", "mac": "..." } }
{ "type": "heartbeat", "deviceId": "ashura-01", "timestamp": 12345 }
```

The server sends:

```json
{ "type": "status", "data": { "status": "registered" } }
{ "type": "command", "data": { "command": "display_message", "text": "Hello!" } }
{ "type": "notification", "data": { "event": "some_event" } }
```

Incoming commands are routed through `MessageRouter` → `DeviceService`. Adding new commands: extend `DeviceService::_handleCommand()`.

---

## Companion Mood System

The `MoodEngine` listens to EventBus events and sets a target `EyeParams`. `CompanionRenderer` lerps the current params toward the target every frame, giving smooth mood transitions.

| Event | Mood |
|-------|------|
| `NotificationReceived` | SURPRISED (700ms flash) |
| `WifiDisconnected` | ANNOYED (3s) |
| `WebSocketRegistered` | HAPPY (8s) |
| `PomodoroStarted` | FOCUSED |
| `PomodoroCompleted` | HAPPY |
| `PomodoroBreak` | HAPPY |
| `SpotifyPlaying` | EXCITED |
| Idle > 40s | BORED |
| Idle > 80s | SLEEPY |
| Any button | Resets idle timer |

---

## Dependencies

| Library | Purpose |
|---------|---------|
| U8g2 | OLED display driver |
| ArduinoJson | JSON parsing/serialization |
| ArduinoWebsockets | WebSocket client |
| ESPmDNS | mDNS for WLED discovery |
| HTTPClient | WLED HTTP API calls |
| Preferences | NVS persistent storage |
| WiFi | WiFi station mode |
