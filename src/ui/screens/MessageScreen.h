#pragma once
#include "IScreen.h"
#include "../DisplayManager.h"
#include <Arduino.h>

// ============================================
// MessageScreen - Fullscreen text notification
//
// Auto-dismisses after timeoutMs (0 = stays until button press)
// On dismiss: UIManager.popScreen() is triggered via onButtonSelect()
// ============================================
class MessageScreen : public IScreen {
public:
  MessageScreen(DisplayManager& display,
                const String& title,
                const String& body,
                unsigned long timeoutMs = 3000)
    : _display(display), _title(title), _body(body), _timeout(timeoutMs) {}

  void onEnter() override {
    _shownAt = millis();
    markDirty();
  }

  // Screens can't popScreen themselves; parent UIManager watches isDirty for
  // a special "wants to pop" signal. Instead we use a callback pattern:
  void onPopped(std::function<void()> cb) { _onPopped = cb; }

  bool wantsPop() const { return _wantsPop; }

  void onButtonSelect() override { _wantsPop = true; }
  void onButtonBack()   override { _wantsPop = true; }

  void update() override {
    // Auto-dismiss check
    if (_timeout > 0 && millis() - _shownAt > _timeout) {
      _wantsPop = true;
    }

    _display.clear();

    _display.setFontLarge();
    _display.drawStr(0, 12, _title.c_str());
    _display.drawLine(0, 14, 128, 14);

    _display.setFontMedium();
    // Simple word-wrap: split on '\n'
    int y = 28;
    String remaining = _body;
    int nl;
    while ((nl = remaining.indexOf('\n')) >= 0 && y < 58) {
      _display.drawStr(0, y, remaining.substring(0, nl).c_str());
      remaining = remaining.substring(nl + 1);
      y += 12;
    }
    if (y < 58) _display.drawStr(0, y, remaining.c_str());

    if (_timeout == 0) {
      _display.setFontSmall();
      _display.drawStr(40, 63, "[OK to dismiss]");
    }

    _display.sendBuffer();
    _dirty = false;
  }

private:
  DisplayManager&       _display;
  String                _title;
  String                _body;
  unsigned long         _timeout;
  unsigned long         _shownAt = 0;
  bool                  _wantsPop = false;
  std::function<void()> _onPopped;
};