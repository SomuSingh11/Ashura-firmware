#pragma once

#include <Arduino.h>
#include <Preferences.h>
#include <U8g2lib.h>

// ================================================================
//  GameScore  —  Shared score + high score for all games
//
//  Stores two values per game in NVS:
//    {key}_hi     → all-time high score
//    {key}_cur    → last completed game score
//
//  Usage:
//    GameScore score("snake");
//    score.load();          // call once in onEnter()
//    score.reset();         // call on game start
//    score.add(1);          // call when points earned
//    score.save();          // call on game over
//    score.draw(u, y);      // call each frame to render bar
//
//  Draw output (at y=0, h=10):
//  ┌──────────────────────────────────────────────────────────┐
//  │  SCORE  142                            HI  389           │
//  └──────────────────────────────────────────────────────────┘
// ================================================================

class GameScore {
    public:
        explicit GameScore(const char* key) : _key(key){}

        // ── NVS ──────────────────────────────────────────────────
        void load() {
            Preferences p;
            p.begin("games", true);
            String hiKey  = String(_key) + "_hi";
            String curKey = String(_key) + "_cur";
            _hi  = p.getInt(hiKey.c_str(), 0);
            _cur = p.getInt(curKey.c_str(), 0);
            p.end(); 
        }

        void save() {
            if(_cur > _hi) _hi = _cur;
            Preferences p;
            p.begin("games", false);
            String hiKey  = String(_key) + "_hi";
            String curKey = String(_key) + "_cur";
            p.putInt(hiKey.c_str(), _hi);
            p.putInt(curKey.c_str(), _cur);
            p.end();
            Serial.println("[Score:" + String(_key) + "] cur=" + _cur + " hi=" + _hi);
        }

        // ── Game control ─────────────────────────────────────────
        void reset() { _cur = 0; _isNewHi = false; }
        
        void add(int points) {
            _cur += points;
            if(_cur > _hi) {
                _hi = _cur;
                _isNewHi = true;
            }
        }

        void set(int val) {
            _cur = val;
            if(_cur > _hi) {
                _hi = _cur;
                _isNewHi = true;
            }
        }

        // ── Accessors ─────────────────────────────────────────────
        int getCurrentScore()   const { return _cur; }
        int getHighScore()      const { return _hi; }
        bool isNewHighScore()   const { return _isNewHi; }

        // ── Draw score bar (top of screen, y=0..9) ───────────────
        void draw(U8G2 &u){
            u.setFont(u8g2_font_5x7_tr);

            // Left: current score
            char cur[16];
            snprintf(cur, sizeof(cur), "SCORE %d", _cur);
            u.drawStr(2, 8, cur);

            // Right: high score (flash if new high)
            char hi[16];
            snprintf(hi, sizeof(hi), "HI %d", _hi);
            int w = u.getStrWidth(hi);

            if (_isNewHi && (millis() / 400) % 2 == 0) {
                // Flash new high score
                u.drawStr(126 - w, 8, hi);
            } else {
                u.drawStr(126 - w, 8, hi);
            }

            // Separator line
            u.drawLine(0, 9, 127, 9);
        }

        // ── Draw game over summary ────────────────────────────────
        void drawGameOver(U8G2& u, int boxX, int boxY, int boxW, int boxH) {
            u.setFont(u8g2_font_5x7_tr);

            char buf[20];
            snprintf(buf, sizeof(buf), "Score: %d", _cur);
            int w = u.getStrWidth(buf);
            u.drawStr(boxX + (boxW - w) / 2, boxY + boxH - 18, buf);

            if (_isNewHi) {
                u.drawStr(boxX + (boxW - u.getStrWidth("NEW BEST!")) / 2,
                        boxY + boxH - 8, "NEW BEST!");
            } else {
                snprintf(buf, sizeof(buf), "Best: %d", _hi);
                w = u.getStrWidth(buf);
                u.drawStr(boxX + (boxW - w) / 2, boxY + boxH - 8, buf);
            }
        }
        
        private:
        const char* _key;            // unique game identifier (e.g. "snake")
        int         _cur     = 0;
        int         _hi      = 0;
        bool        _isNewHi = false;
};