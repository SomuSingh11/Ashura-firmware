#pragma once 
#include "../../core/DisplayManager.h";

// ================================================================
//  IOverlay  —  Interface for screen overlays
//
//  Overlays render ON TOP of the current screen without
//  replacing it. The screen underneath keeps running normally.
//
//  UIManager holds one active overlay at a time.
//  Overlay is cleared when expired() returns true.
//
//  Implementing classes:
//    ToastOverlay  — notification banner, 3s auto-dismiss
//
//  Future overlays could include:
//    ConfirmDialog  — "Are you sure?" prompt
//    InputPrompt    — quick yes/no over current screen
// ================================================================

class IOverlay {
    public:
        // Render overlay onto display — called every frame while active
        virtual void render     (DisplayManager& display) = 0;

        // Return true when overlay should be removed
        virtual bool expired    () = 0;

        // Return true if user pressed SELECT during this overlay
        // UIManager checks this to trigger inbox navigation
        virtual bool wantsAction() = 0;

        // Called by UIManager when SELECT is pressed while overlay active
        virtual void onSelect   () = 0;

        // Destructor
        virtual      ~IOverlay  () = default;
};