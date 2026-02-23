#pragma once
#include <Arduino.h>
#include "Animation.h"
#include "../../ui/DisplayManager.h"

// ============================================
// AnimationPlayer - Frame-by-frame GIF player
//
// Reads frame data from PROGMEM via pgm_read_ptr.
// Timing controlled by Animation::delayMs.
// Call update() every loop tick.
// ============================================

class AnimationPlayer {
    public :
        AnimationPlayer() = default;

        void setAnimation(const Animation* anim){
            _anim = anim;
            _currentFrame=0;
            _lastFrameTime=0;
            Serial.println("▶ Vibe: " + String(anim->name));
        }

        void update(DisplayManager& display){
            if(!_anim) return;

            
        }

        const Animation* currentAnim() const { return _anim; }
        bool isPlaying() const { return _anim != nullptr; }

    private :
        const Animation* _anim          = nullptr;
        int              _currentFrame  = 0;
        unsigned long    _lastFrameTime = 0;
};