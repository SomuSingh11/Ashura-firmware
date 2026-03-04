#pragma once
#include <Arduino.h>
#include "EventBus.h"
#include "config.h"

// ================================================================
//  NotificationManager  —  Session-only notification queue
//
//  Ring buffer of max 10 notifications. Oldest dropped when full.
//  Notifications are lost on reboot (session-only by design).
//  Persistence can be added later for email/messages if needed.
//
//  Types:
//    SYSTEM   — WiFi failed, server unavailable, OS events
//    ALERT    — time-sensitive warnings
//    MESSAGE  — incoming messages from server / other devices
//
//  Usage:
//    Notifs().push(NotifType::SYSTEM, "WiFi", "Failed after 8 attempts");
//    int n = Notifs().unreadCount();
//    const Notification* latest = Notifs().latest();
//    Notifs().markAllRead();
//
//  HomeScreen polls unreadCount() each frame for the dot indicator.
//  NotificationDetailScreen calls markAllRead() on enter.
// ================================================================

enum class NotificationType {
    SYSTEM,             // WiFi/WS failures, OS events
    ALERT,              // time-sensitive warnings 
    MESSAGE,            // incoming messages from server / other devices
};

struct Notification {
    NotificationType    type        = NotificationType::SYSTEM;
    String              title;
    String              body;
    unsigned long       timestamp   = 0;
    bool                read        = false;
};

class NotificationManager {
    public:
        // ── Singleton ─────────────────────────────────────────────
        static NotificationManager& instance() {
            static NotificationManager _mgr;
            return _mgr;
        }

        // ── Push a new notification ───────────────────────────────
        // Drops oldest if buffer is full.
        // Publishes NotificationPushed to EventBus.
        void push(NotificationType type, const String& title, const String& body){
            Notification notif;
            notif.type = type;
            notif.title = title;
            notif.body = body;
            notif.timestamp = millis();
            notif.read = false;

            if(_count < MAX_NOTIFICATION_BUFFER) {
                _buf[_count++] = notif;
            } else {
                // Shift left -> drop oldest
                for(int i=0; i<MAX_NOTIFICATION_BUFFER-1; i++){
                    _buf[i] = _buf[i+1];
                };
                _buf[MAX_NOTIFICATION_BUFFER-1] = notif;
            }

            Serial.println("[Notif] " + _typeStr(type) + " | " + title + ": " + body);
            Bus().publish(AppEvent::NotificationPushed, title);
        }

        // ── Accessors ─────────────────────────────────────────────
        int  count()    const { return _count; }
        bool isEmpty()  const { return _count == 0; }

        int unreadCount() const {
            int n=0;
            for(int i=0; i<_count; i++){
                if(!_buf[i].read) n++;
            }
            return n;
        }

        // Most recent notification (for ticker)
        const Notification* latest() const {
            if(_count == 0) return nullptr;
            return &_buf[_count-1];
        }

        // Most recect unread (for badge dot)
        const Notification* latestUnread() const {
            for(int i=_count-1; i>=0; i--){
                if(!_buf[i].read) return &_buf[i];
            }
            return nullptr;
        }

        // Get by index(0 = oldest, count-1 = latest)
        const Notification* get(int index) const {
            if(index < 0 || index >= _count) return nullptr;
            return &_buf[index];
        };

        // ── Mark read ─────────────────────────────────────────────
        void markRead(int index) {
            if(index >= 0 && index < _count) _buf[index].read = true;
        }

        void markAllRead(){
            for(int i=0; i<_count; i++) _buf[i].read = true;
        }

        // ── Clear ─────────────────────────────────────────────────
        void clear() { _count=0; }

        // ── Helpers ───────────────────────────────────────────────
        static String typeLabel(NotificationType t){
            switch(t) {
                case NotificationType::SYSTEM: return "SYSTEM";
                case NotificationType::ALERT:   return "ALERT";
                case NotificationType::MESSAGE: return "MSG";
            }
            return "";
        }

        // Friendly relative timestamp string
        String timeAgo(int index) const {
            if(index<0 || index>=_count) return "";
            unsigned long age = (millis()-_buf[index].timestamp) / 1000;
            if (age < 60)   return String(age) + "s ago";
            if (age < 3600) return String(age / 60) + "m ago";
            return String(age / 3600) + "h ago";
        }

    private:
        NotificationManager() = default;
        Notification    _buf[MAX_NOTIFICATION_BUFFER];
        int             _count = 0;

        static String _typeStr(NotificationType t){
            switch(t){
                case NotificationType::SYSTEM:  return "SYS";
                case NotificationType::ALERT:   return "ALT";
                case NotificationType::MESSAGE: return "MSG";
            }

            return "?";
        }
};

// Global shorthand
inline NotificationManager& NotifMgr() { return NotificationManager::instance(); }