#pragma once
#include "IService.h"
#include "../ui/UIManager.h"

// ============================================
// WLEDService - Future WLED integration
//
// Step 1: Register in AppManager:
//   _router.registerService(new WLEDService(_ui, _display));
//
// Step 2: Implement fetchPresets() using HTTPClient:
//   HTTPClient http;
//   http.begin("http://" + WLED_IP + "/json/presets");
//   then parse JSON array into MenuItem list
//
// Step 3: On incoming WS command "wled_*", this service handles it
//
// Step 4: Add "WLED" to main menu in AppManager to open WLEDMenuScreen
// ============================================
class WLEDService : public IService {
public:
  WLEDService(UIManager& ui) : _ui(ui) {}

  void init() override {
    // TODO: Fetch presets on startup and cache them
    Serial.println("WLEDService: init (stub)");
  }

  bool handleMessage(const JsonDocument& doc) override {
    const char* type = doc["type"];
    if (!type) return false;

    // Handle messages targeted at WLED, e.g. "wled_command"
    if (strcmp(type, "wled_command") == 0) {
      const char* action = doc["data"]["action"];
      Serial.println("WLED action: " + String(action ? action : "null"));
      // TODO: HTTP POST to WLED API
      return true;
    }

    return false;
  }

  const char* getName() const override { return "WLEDService"; }

private:
  UIManager& _ui;
  // TODO: String _wledIP = WLED_HOST;
  // TODO: std::vector<MenuItem> _cachedPresets;
};