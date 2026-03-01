#pragma once
#include <Preferences.h>
#include <Arduino.h>

// ============================================================
//  AshuraPrefs  —  Persistent user preferences via ESP32 NVS
//
//  Wraps ESP32's Preferences library (key-value NVS flash store).
//  Keys are namespaced under "ashura" to avoid collisions.
//
//  Current keys:
//    aura_ss    → int  screensaver vibe index (default 0)
//    aura_boot  → int  boot animation index   (default 0)
//
//  Usage:
//    AshuraPrefs::setScreensaver(1);
//    int idx = AshuraPrefs::getScreensaver(); // → 1
//
//  NVS survives power cycles and OTA updates.
//  Flash wear: NVS uses wear-levelling, safe for frequent writes.
// ============================================================

class AshuraPrefs {
    public:

        // ── Screensaver index ────────────────────────────────────
        static int getScreensaver() {
            Preferences p;
            p.begin("ashura_vibes", true);  // read-only
            int v = p.getInt("screensaver", 0);
            p.end();
            return v;
        }

        static void setScreensaver(int index){
            Preferences p;
            p.begin("ashura_vibes", false); // read-write
            p.putInt("screensaver", index);
            p.end();
            Serial.println("[Prefs] Screensaver → " + String(index));
        }

        // ── Boot animation index ─────────────────────────────────
        static int getBoot() {
            Preferences p;
            p.begin("ashura_vibes", true);
            int v = p.getInt("vibes_boot", 0);
            p.end();
            return v;
        }

        static void setBoot(int index) {
            Preferences p;
            p.begin("ashura_vibes", false);
            p.putInt("vibes_boot", index);
            p.end();
            Serial.println("[Prefs] Boot → " + String(index));
        }

        // ── HomeScreen animation index ───────────────────────────
        static int getHomeScreen() {
            Preferences p;
            p.begin("ashura_vibes", true);
            int v = p.getInt("vibes_home", 0);
            p.end();
            return v;
        }

        static void setHomeScreen(int index) {
            Preferences p;
            p.begin("ashura_vibes", false);
            p.putInt("vibes_home", index);
            p.end();
            Serial.println("[Prefs] HomeScreen → " + String(index));
        }

        // ── Reset all prefs to defaults ──────────────────────────
        static void resetAll() {
            Preferences p;
            p.begin("ashura_vibes", false);
            p.clear();
            p.end();
            Serial.println("[Prefs] Reset to defaults");
        }
};