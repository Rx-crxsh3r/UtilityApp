// src/settings/overlay_manager.h
// Overlay style management header

#pragma once
#include <windows.h>
#include "../overlay.h"  // Use the existing OverlayStyle enum

class OverlayManager {
private:
    OverlayStyle currentStyle;
    bool isDirty;
    
public:
    OverlayManager();
    
    // Style management
    void SetStyle(OverlayStyle style);
    OverlayStyle GetStyle() const { return currentStyle; }
    
    // UI handling
    void InitializeRadioButtons(HWND hDialog, int firstRadioId);
    void HandleRadioButtonClick(HWND hDialog, int clickedId, int firstRadioId);
    bool UpdateFromDialog(HWND hDialog, int firstRadioId);
    
    // Change tracking
    bool IsDirty() const { return isDirty; }
    void ClearDirty() { isDirty = false; }
    
    // Validation
    bool IsValidStyle(int style) const;
    const char* GetStyleDescription(OverlayStyle style) const;
};

// Global instance
extern OverlayManager g_overlayManager;
