#include "WledManager.h"
#include <Preferences.h>

// ================================================================
// WledManager is responsible for:
//         1. Storing discovered devices
//         2. Selecting an active device
//         3. Talking to it through _client
//         4. Caching its state and effects
//         5. Persisting everything in NVS (ESP Preferences)
//  So this class is:
//          "Device registry + persistence layer + active-session manager"
// ================================================================


// ── Lifecycle ─────────────────────────────────────────────────

void WledManager::begin() {
    loadFromNVS();
    Serial.println("[WledManager] Ready — " + String(_devices.size()) + " saved device(s)");
}


// ── Devices ───────────────────────────────────────────────────

const WledDevice* WledManager::activeDevice() const {
    if(_activeIndex < 0 || _activeIndex >= (int)_devices.size()) return nullptr;
    return &_devices[_activeIndex];
}

String WledManager::effectName(int index) const {
    if(index < 0 || index >= (int)_effects.size()) return "Unknown";
    return _effects[index];
}


// ── Connect ───────────────────────────────────────────────────

bool WledManager::connect(int index){
    if(index < 0 || index >= (int)_devices.size()){
        Serial.println("[WledManager] connect() — invalid index " + String(index));
        return false;
    }

    _activeIndex = index;
    _client.setDevice(_devices[index]);

    _effects.clear();
    _state.valid = false;

    Serial.println("[WledManager] Connecting to " + _devices[index].name + " @ " + _devices[index].ip);

    // Fetch current state
    // _State -> populated | _state.valid = true
    if(!_client.fetchState(_state)){
        Serial.println("[WledManager] Failed to fetch state");
        return false;
    }

    // Fetch effects list and cache it
    if (_client.fetchEffects(_effects)) {
        Serial.println("[WledManager] " + String(_effects.size()) + " effects loaded");
    } else {
        Serial.println("[WledManager] Warning: could not load effects list");
    }

    // Persist active index
    Preferences p;
    p.begin("wled",  false);
    p.putInt("activeIdx", _activeIndex);
    p.end();

    return true;
}


// ── Discovery ─────────────────────────────────────────────────

int WledManager::discover() {
    // Scan for devices, merge with existing list, persist new list.
    Serial.println("[WledManager] Starting discovery...");

    std::vector<WledDevice> found;
    WledDiscovery::scan(found, _client);

    int newDevicesCount = 0;

    for(WledDevice dev: found){
        if(!_hasDevice(dev.ip)){
            if((int)_devices.size() < WLED_MAX_DEVICES) {
                _devices.push_back(dev);
                newDevicesCount++;
                Serial.println("[WledManager] New device added: " + dev.name + " @ " + dev.ip);
            } else {
                Serial.println("[WledManager] Device limit reached, skipping: " + dev.name + " @ " + dev.ip);
            }
        } else {
            // Update existing device info (e.g. name) in case it changed
            for(WledDevice& existing: _devices){
                if(existing.ip == dev.ip){
                    existing.name = dev.name;
                    break;
                }
            }
            Serial.println("[WledManager] Device already known, skipping: " + dev.name + " @ " + dev.ip);
        }
    }

    if(newDevicesCount > 0) {
        saveToNVS();
    }

    Serial.println("[WledManager] Discovery done — " + String(newDevicesCount) + " new device(s)");
    return newDevicesCount;

}


// ── Remove device ─────────────────────────────────────────────

void WledManager::removeDevice(int index) {
    if(index < 0|| index >= (int)_devices.size()) return;

    Serial.println("[WledManager] Removing: " + _devices[index].name);
    _devices.erase(_devices.begin() + index);

    // clamp active index
    if(_activeIndex >= (int)_devices.size()) {
        _activeIndex = max(0, (int)_devices.size() - 1);
    }

    saveToNVS();
}


// ── NVS ───────────────────────────────────────────────────────

void WledManager::loadFromNVS() {
    Preferences p;
    p.begin("wled",  true); // read-only

    // Load active index
    _activeIndex = p.getInt("activeIndex", 0);
    int deviceCount = p.getInt("deviceCount", 0);
    deviceCount = min(deviceCount, WLED_MAX_DEVICES);

    _devices.clear();

    for(int i=0; i<deviceCount; i++){
        WledDevice dev;
        dev.ip = p.getString  (("dev_" + String(i) + "_ip").c_str(), "");
        dev.name = p.getString(("dev_" + String(i) + "_name").c_str(), "");
        dev.port = p.getInt   (("dev_" + String(i) + "_port").c_str(), 80);

        if(dev.isValid()){
            _devices.push_back(dev);
        }
    }

    p.end();

    // Clamp active index to valid range
    if (_activeIndex >= (int)_devices.size()) _activeIndex = 0;

    Serial.println("[WledManager] Loaded " + String(_devices.size()) + " devices from NVS");
}

void WledManager::saveToNVS() {
    Preferences p;
    p.begin("wled",  false);    // read-write

    int count = min((int)_devices.size(), WLED_MAX_DEVICES);

    p.putInt("activeIndex", _activeIndex);
    p.putInt("deviceCount", count);

    for(size_t i=0; i<count; i++){
        const WledDevice& dev = _devices[i];
        p.putString(("dev_" + String(i) + "_ip").c_str(), dev.ip);
        p.putString(("dev_" + String(i) + "_name").c_str(), dev.name);
        p.putInt   (("dev_" + String(i) + "_port").c_str(), dev.port);
    }

    p.end();

    Serial.println("[WledManager] Saved " + String(count) + " devices to NVS");
}

// ── Private helpers ───────────────────────────────────────────

bool WledManager::_hasDevice(const String&ip) const {
    for(const WledDevice& dev: _devices){
        if(dev.ip == ip) return true;
    }
    return false;
}