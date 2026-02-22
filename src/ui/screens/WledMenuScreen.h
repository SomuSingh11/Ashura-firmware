#pragma once
#include "MenuScreen.h"
#include "../DisplayManager.h"
#include <Arduino.h>

// ============================================
// WLEDMenuScreen - WLED preset / effect browser
//
// Currently shows placeholder items.
// TODO: fetch presets via HTTP GET to WLED API
//       GET http://<wled-ip>/json/presets
//       then build _items dynamically
// ============================================
class WLEDMenuScreen : public IScreen {
public:
  WLEDMenuScreen(DisplayManager& display, std::function<void()> onBack)
    : _display(display), _onBack(onBack) {}

  void onEnter() override {
    _buildMenu();
    markDirty();
  }

  void onButtonDown() override {
    _cursor = (_cursor + 1) % _items.size();
    markDirty();
  }

  void onButtonUp() override {
    _cursor = (_cursor - 1 + _items.size()) % _items.size();
    markDirty();
  }

  void onButtonSelect() override {
    if (_cursor < _items.size()) {
      _items[_cursor].onSelect();
    }
  }

  void update() override {
    _display.clear();
    _display.setFontLarge();
    _display.drawStr(0, 10, "WLED Presets");
    _display.drawLine(0, 12, 128, 12);

    _display.setFontMedium();
    const int ROW_H = 12, START_Y = 25, MAX_VIS = 4;
    int windowStart = (_cursor / MAX_VIS) * MAX_VIS;

    for (int i = 0; i < MAX_VIS; i++) {
      int idx = windowStart + i;
      if (idx >= (int)_items.size()) break;
      int y = START_Y + i * ROW_H;
      if (idx == (int)_cursor) _display.drawStr(0, y, ">");
      _display.drawStr(10, y, _items[idx].label.c_str());
    }

    _display.sendBuffer();
    _dirty = false;
  }

private:
  void _buildMenu() {
    _items.clear();

    // TODO: Replace these stubs with real HTTP fetch from WLED
    // For now, placeholder presets
    _items.push_back({ "Rainbow",    []() { Serial.println("WLED: Rainbow"); } });
    _items.push_back({ "Fire",       []() { Serial.println("WLED: Fire");    } });
    _items.push_back({ "Solid White",[]() { Serial.println("WLED: White");   } });
    _items.push_back({ "Off",        []() { Serial.println("WLED: Off");     } });
    _items.push_back({ "< Back",     [this]() { if (_onBack) _onBack(); }     });
  }

  DisplayManager&        _display;
  std::vector<MenuItem>  _items;
  int                    _cursor = 0;
  std::function<void()>  _onBack;
};