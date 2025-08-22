// src/overlay.h
// Screen overlay system for lock visual feedback

#pragma once
#include <windows.h>

// Overlay styles
enum OverlayStyle {
    OVERLAY_BLUR = 0,
    OVERLAY_DIM = 1,
    OVERLAY_BLACK = 2,
    OVERLAY_NONE = 3
};

class ScreenOverlay {
private:
    HWND hOverlayWindow;
    HBRUSH hBackgroundBrush;
    OverlayStyle currentStyle;
    bool isVisible;
    
    // Window procedure for overlay
    static LRESULT CALLBACK OverlayWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    
    // Helper functions
    void CreateOverlayWindow();
    void UpdateOverlayStyle();
    void SetupBlurEffect();
    void SetupDimEffect();
    void SetupBlackEffect();
    
public:
    ScreenOverlay();
    ~ScreenOverlay();
    
    // Main overlay functions
    void ShowOverlay(OverlayStyle style);
    void HideOverlay();
    void SetStyle(OverlayStyle style);
    bool IsVisible() const { return isVisible; }
    OverlayStyle GetStyle() const { return currentStyle; }
};

// Global overlay instance
extern ScreenOverlay g_screenOverlay;
