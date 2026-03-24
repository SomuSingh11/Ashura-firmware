#pragma once
#include "IOverlay.h"
#include "../../core/NotificationManager.h"
 
// ================================================================
//  ToastOverlay  —  Non-intrusive notification banner
//
//  Renders at the bottom of whatever screen is currently showing.
//  Auto-dismisses after TOAST_DURATION_MS.
//  User can press SELECT during toast to open inbox.
//
//  Layout (128x64, toast occupies bottom 12px):
//
//  ┌──────────────────────────────────────────────┐
//  │  [current screen — fully visible]            │
//  │                                              │
//  │                                              │
//  ┌──────────────────────────────────────────────┐
//  │● ALERT   WiFi Failed                   now  │  y=52..63
//  └──────────────────────────────────────────────┘
//
//  Dot indicators:
//    ALERT   → filled disc
//    MESSAGE → outline circle
//    SYSTEM  → no dot (stays subtle)
// ================================================================

class ToastOverlay : public IOverlay {
    public:
    explicit ToastOverlay(const Notification& notif):
                _notif(notif),
                _shownAt(millis()),
                _wantsAction(false)
            {}

    // ── IOverlay ─────────────────────────────────────────────

    void render(DisplayManager& display) override {
        auto& u = display.raw();

        // Background
        u.setDrawColor(0);
        u.drawBox(0, 51, 128, 13);

        // Top header
        u.setDrawColor(1);
        u.drawHLine(0, 51, 128);

        // Dot indicator for type
        constexpr int dotX = 4;
        constexpr int dotY = 57;
        switch (_notif.type) {
            case NotificationType::ALERT:
                u.drawDisc(dotX, dotY, 2);   // filled
                break;
            case NotificationType::MESSAGE:
                u.drawCircle(dotX, dotY, 2); // outline
                break;
            case NotificationType::SYSTEM:
                break;                        // no dot
        }

        // Title text
        u.setFont(u8g2_font_5x7_tr);
        int textX = (_notif.type == NotificationType::SYSTEM) ? 2 : 9;

        // Truncate to fit — leave room for time on right (~5 chars = 25px)
        String title = _notif.title;
        if (title.length() > 17) {
            title = title.substring(0, 16) + "~";
        }
        u.drawStr(textX, 62, title.c_str());
 
        // Time ago — right aligned 
        String t = _shortTimeAgo();
        int tw = u.getStrWidth(t.c_str());
        u.drawStr(127 - tw, 62, t.c_str());
 
        u.setDrawColor(1); // restore
    }

    bool expired() override {
        return (millis() - _shownAt) >= TOAST_DURATION_MS;
    }

    bool wantsAction() override { _wantsAction; }
    void onSelect() override { _wantsAction = true; }

    // UIManager uses this to scroll inbox to the right notification
    uint32_t notifId() const { return _notif.id; }

    private:
        Notification    _notif;
        uint32_t        _shownAt;
        bool            _wantsAction;

        // Compact time for toast — "now", "5m", "2h"
        String _shortTimeAgo() const {
            uint32_t elapsed = (millis() - _notif.timestamp) / 1000;
            if (elapsed < 60) return "now";
            if (elapsed < 3600) return String(elapsed / 60)   + "m";
            return String(elapsed / 3600) + "h";
        }
};