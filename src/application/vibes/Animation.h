#pragma once 
#include <Arduino.h>

// ============================================================
//  Animation  —  A named PROGMEM frame sequence
//
//  Each frame is a 128×64 1-bit XBM bitmap = 1024 bytes.
//  All frame arrays MUST be declared PROGMEM.
//
//  Example (in your gif header):
//    const unsigned char my_frame_001[] PROGMEM = { ... };
//    const unsigned char* my_frames[] PROGMEM    = { my_frame_001, ... };
//    const Animation my_anim = { my_frames, 27, 80, "My Anim" };
// ============================================================

struct Animation {
    const unsigned char* const*     frames;         // PROGMEM array of PROGMEM frame pointers
    int                             frameCount;     // total frames
    int                             frameDelayMs;   // ms per frame
    const char*                     name;           // display name (for picker UI)
};