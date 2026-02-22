#pragma once
#include <Arduino.h>
#include <time.h>

// ============================================
// TimeManager - Non-blocking NTP time sync
//
// Usage:
//   Time().sync()       → call once after WiFi connects (returns instantly)
//   Time().isSynced()   → true once NTP has responded
//   Time().getHH/MM/SS  → always returns something (uptime fallback before sync)
//
// Singleton: access via Time() from anywhere
// ============================================

class TimeManager {
    public:
        //Singleton Pattern
        static TimeManager& instance() {
            static TimeManager tm;
            return tm;
        }

        void sync(){
            configTime(UTC_OFFSET_SEC, 0, "pool.ntp.org", "time.google.com", "time.nist.gov");
            Serial.println("⏱ NTP sync started...");

            // Wait upto 5 sec
            // struct tm timeinfo;
            // int attempts = 0;

            // while(!getLocalTime(&timeinfo) && attempts < 50){
            //     delay(100);
            //     yield();
            //     attempts++;
            // }

            // if(attempts < 50){
            //     _synced = true;
            //     Serial.println("✅ NTP synced!");
            // } else {
            //     Serial.println("⚠️ NTP sync failed, using uptime");
            // }
        }

        bool isSynced() const { 
            struct tm t;
            return getLocalTime(&t, 0); // non-blocking check
         }

        // Returns current hour (0-24)
        int getHH() const {
            struct tm t;
            if(getLocalTime(&t, 0)) return t.tm_hour; // fill the struct with current real time
            return (millis() / 3600000) % 24; // fallback
        }
        
        // Returns current minute (0-59)
        int getMM() const {
            struct tm t;
            if(getLocalTime(&t, 0)) return t.tm_min;
            return (millis() / 60000) % 60;
        }

        // Return current second (0-59)
        int getSS() const {
            struct tm t;
            if (getLocalTime(&t, 0)) return t.tm_sec;
            return (millis() / 1000) % 60;
        }

        String getTimeString() const {
            char buf[6];
            sprintf(buf, "%02d:%02d", getHH(), getMM());
            return String(buf);
        }

        String getTimeStringFull() const {
            char buf[9];
            sprintf(buf, "%02d:%02d:%02d", getHH(), getMM(), getSS());
            return String(buf);
        }

        String getDateString() const {
            if (!isSynced()) return "Syncing...";
            struct tm t;
            if (!getLocalTime(&t, 0)) return "Syncing...";
            char buf[12];
            sprintf(buf, "%02d/%02d/%04d", t.tm_mday, t.tm_mon + 1, t.tm_year + 1900);
            return String(buf);
        }

    private:
        TimeManager()                       = default;
        static const long   UTC_OFFSET_SEC  = 19800;  // Change this to your timezone (IST +5:30)
};

inline TimeManager& Time() { return TimeManager::instance(); }