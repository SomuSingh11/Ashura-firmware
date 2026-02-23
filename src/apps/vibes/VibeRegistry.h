#pragma once
#include "Animation.h"

// ============================================
// VibeRegistry - Master list of all vibes
//
// To add a new vibe:
//   1. Put your .h file in src/vibes/gifs/
//   2. #include it below
//   3. Add one line to ALL_VIBES[]
//   Nothing else changes — menu updates automatically.
// ============================================

#include "./gifs/kakashi.h"
#include "./gifs/shoyo.h"

struct VibeEntry {
    const char* name;
    const Animation* animation;
};

static const VibeEntry ALL_VIBES[] = {
    {"Shoyo", &shoyo},
    {"Kakashi", &kakashi}
};