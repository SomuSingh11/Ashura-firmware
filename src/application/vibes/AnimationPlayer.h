#pragma once
#include <Arduino.h>
#include "Animation.h"
#include "../../core/DisplayManager.h"

// ============================================================
//  AnimationPlayer  —  Shared PROGMEM frame driver
//
//  Used by VibePlayerScreen and VibePreviewScreen.
//  Handles all frame timing and PROGMEM reads in one place.
//
//  Two draw modes:
//
//  1. update(display)
//     Full autonomous render — clears buffer, draws frame, sends.
//     Use this when nothing else is drawn on top (VibePlayerScreen).
//
//  2. tick()  +  drawFrame(display)
//     Separated — tick() advances the frame counter on schedule,
//     drawFrame() writes the XBM into the current buffer only.
//     The caller controls clearBuffer() and sendBuffer().
//     Use this when you need to draw UI on top (VibePreviewScreen).
//
//  PROGMEM notes:
//    frames[] array is PROGMEM → pgm_read_ptr() to dereference
//    each frame buffer is PROGMEM → drawXBM reads it natively
//    zero RAM cost — no frame data ever copied to heap
// ============================================================

class AnimationPlayer {
    public:
        AnimationPlayer() = default;

        void setAnimation(const Animation* animation) {
            _animation  = animation;
            _frame      = 0;
            _lastTick   = 0;
            if (animation) Serial.println("[Aura] > " + String(animation->name));
        }

        void reset() {
            _frame      = 0;
            _lastTick   = millis();
        }

        // ── Mode 1: fully autonomous ─────────────────────────────
        // Advances frame + clears buffer + draws + sends. One call does all.
        // Returns true if a new frame was drawn this tick.
        
        bool update(DisplayManager& display) {
            if(!_animation) return false;
            bool advanced = _advance();
            auto& u = display.raw();
            u.clearBuffer();
            _drawCurrentFrame(u);
            u.sendBuffer();
            return advanced;
        }

        // ── Mode 2a: advance frame counter only ──────────────────
        // Returns true if the frame changed — caller should redraw.
        bool tick(){
            if(!_animation) return false;
            return _advance();
        }

        // ── Mode 2b: draw current frame into buffer only ─────────
        // Does NOT clear or send — caller owns the buffer lifecycle.
        // Call after clearBuffer(), call sendBuffer() after your overlays.
        void drawFrame(DisplayManager& display) {
            if (!_animation) return;
            _drawCurrentFrame(display.raw());
        }

        // ── Accessors ─────────────────────────────────────────────
        bool                isPlaying()        const { return _animation != nullptr; }
        int                 currentFrame()  const { return _frame; }
        const Animation*    anim()          const { return _animation; }



    private:
        const Animation*    _animation  = nullptr;
        int                 _frame      = 0;
        unsigned long       _lastTick   = 0;

        bool _advance() {
            unsigned long now = millis();
            if(now - _lastTick >= (unsigned long)_animation->frameDelayMs){
                _lastTick = now;
                _frame = (_frame+1) % _animation->frameCount;
                return true;
            }
            return false;
        }

        void _drawCurrentFrame(U8G2& u) {
            const unsigned char* frame = _animation->frames[_frame];
            u.drawBitmap(0, 0, 16, 64, frame);
        }
};