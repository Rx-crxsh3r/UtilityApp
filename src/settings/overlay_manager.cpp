// src/settings/overlay_manager.cpp
// Overlay style management implementation

#include "overlay_manager.h"
#include "../resource.h"

// Define radio button IDs if not already defined
#ifndef IDC_RADIO_BLUR
#define IDC_RADIO_BLUR    240
#define IDC_RADIO_DIM     241  
#define IDC_RADIO_BLACK   242
#define IDC_RADIO_NONE    243
#endif

// Global instance
OverlayManager g_overlayManager;

OverlayManager::OverlayManager() : currentStyle(OVERLAY_BLUR), isDirty(false) {
}

void OverlayManager::SetStyle(OverlayStyle style) {
    if (IsValidStyle(style) && currentStyle != style) {
        currentStyle = style;
        isDirty = true;
    }
}

void OverlayManager::InitializeRadioButtons(HWND hDialog, int firstRadioId) {
    // Ensure only the current style is selected using proper CheckRadioButton
    CheckRadioButton(hDialog, IDC_RADIO_BLUR, IDC_RADIO_NONE, IDC_RADIO_BLUR + currentStyle);
}

void OverlayManager::HandleRadioButtonClick(HWND hDialog, int clickedId, int firstRadioId) {
    int styleIndex = clickedId - IDC_RADIO_BLUR;
    
    if (IsValidStyle(styleIndex)) {
        // Ensure mutual exclusivity using proper CheckRadioButton
        CheckRadioButton(hDialog, IDC_RADIO_BLUR, IDC_RADIO_NONE, clickedId);
        
        OverlayStyle newStyle = (OverlayStyle)styleIndex;
        if (newStyle != currentStyle) {
            currentStyle = newStyle;
            isDirty = true;
        }
    }
}

bool OverlayManager::UpdateFromDialog(HWND hDialog, int firstRadioId) {
    bool wasChanged = false;
    
    for (int i = 0; i < 4; i++) {
        if (IsDlgButtonChecked(hDialog, IDC_RADIO_BLUR + i) == BST_CHECKED) {
            OverlayStyle newStyle = (OverlayStyle)i;
            if (newStyle != currentStyle) {
                currentStyle = newStyle;
                isDirty = true;
                wasChanged = true;
            }
            break; // Only one should be checked
        }
    }
    
    return wasChanged;
}

bool OverlayManager::IsValidStyle(int style) const {
    return style >= OVERLAY_BLUR && style <= OVERLAY_NONE;
}

const char* OverlayManager::GetStyleDescription(OverlayStyle style) const {
    switch (style) {
        case OVERLAY_BLUR: return "Apply a blur effect to the background";
        case OVERLAY_DIM: return "Darken the background with transparency";
        case OVERLAY_BLACK: return "Show a solid black overlay";
        case OVERLAY_NONE: return "No visual overlay (input still locked)";
        default: return "Unknown style";
    }
}
