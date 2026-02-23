#pragma once
#include <Arduino.h>

// ============================================
// Animation - data structure for a single GIF
//
// All frame arrays MUST be PROGMEM.
// Each frame is a 128x64 bitmap = 1024 bytes.
// 27 frames = ~27KB of flash (4MB available).
// ============================================

struct Animation {
    const unsigned char* const*   frames;      // Pointer to PROGMEM array of frames
    int                           frameCount;  // Total number of frames in the animation
    int                           frameDelay;  // Delay between frames in milliseconds
    const char*                   name;        // Name of the animation (for display purposes)
};