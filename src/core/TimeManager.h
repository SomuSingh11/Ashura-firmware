#pragma once
#include <Arduino.h>
#include <time.h>
#include "config.h"

// ============================================================
//  TimeManager  —  Non-blocking NTP time service
// ============================================================

class TimeManager {
    public:
        //Singleton Pattern
        static TimeManager& instance() {
            static TimeManager tm;
            return tm;
        }

        void sync() {
            configTime(UTC_OFFSET_SEC, 0, "pool.ntp.org", "time.google.com", "time.nist.gov");
            Serial.println("[Time] NTP sync requested");
            _syncRequested = true;
        }   

        // Poll once per loop — marks synced after first successful getLocalTime
        void update() {
            if (_synced || !_syncRequested) return;
            struct tm ti;
            if (getLocalTime(&ti, 0)) {
                _synced = true;
                Serial.println("[Time] ✅ NTP synced");
            }
        }

        bool isSynced() const { return _synced; }

        int getHH() const { return _get().tm_hour; }
        int getMM() const { return _get().tm_min;  }
        int getSS() const { return _get().tm_sec;  }

        String getDateString() const {
            char buf[6];
            struct tm ti = _get();
            sprintf(buf, "%02d:%02d", ti.tm_hour, ti.tm_min);
            return String(buf);
        }

    private:
        TimeManager()                           = default;
        bool                    _synced         = false;
        bool                    _syncRequested  = false;
        mutable unsigned long   _bootMs         = 0;

        struct tm _get() const {
            struct tm ti;
            if(!getLocalTime(&ti, 0)) {

                unsigned long s = millis() / 1000;
                ti.tm_hour = (s / 3600) % 24;
                ti.tm_min = (s / 60) % 60;
                ti.tm_sec = s % 60;
            }
            return ti;
        }
};

inline TimeManager& Time() { return TimeManager::instance(); }