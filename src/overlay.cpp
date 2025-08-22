// src/overlay.cpp
// Screen overlay system implementation

#include "overlay.h"
#include "settings.h"
#include <dwmapi.h>

// Global overlay instance
ScreenOverlay g_screenOverlay;

// Overlay window class name
static const char OVERLAY_CLASS_NAME[] = "UtilityAppOverlay";

ScreenOverlay::ScreenOverlay() 
    : hOverlayWindow(nullptr), hBackgroundBrush(nullptr), 
      currentStyle(OVERLAY_BLUR), isVisible(false) {
}

ScreenOverlay::~ScreenOverlay() {
    HideOverlay();
    if (hBackgroundBrush) {
        DeleteObject(hBackgroundBrush);
    }
    if (hOverlayWindow) {
        DestroyWindow(hOverlayWindow);
    }
}

void ScreenOverlay::ShowOverlay(OverlayStyle style) {
    if (style == OVERLAY_NONE) {
        HideOverlay();
        return;
    }
    
    currentStyle = style;
    
    if (!hOverlayWindow) {
        CreateOverlayWindow();
    }
    
    UpdateOverlayStyle();
    
    // Show the overlay window
    ShowWindow(hOverlayWindow, SW_SHOW);
    SetWindowPos(hOverlayWindow, HWND_TOPMOST, 0, 0, 0, 0, 
                SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
    UpdateWindow(hOverlayWindow);
    
    isVisible = true;
}

void ScreenOverlay::HideOverlay() {
    if (hOverlayWindow && isVisible) {
        ShowWindow(hOverlayWindow, SW_HIDE);
        isVisible = false;
    }
}

void ScreenOverlay::SetStyle(OverlayStyle style) {
    currentStyle = style;
    if (isVisible && style != OVERLAY_NONE) {
        UpdateOverlayStyle();
        InvalidateRect(hOverlayWindow, NULL, TRUE);
    } else if (style == OVERLAY_NONE) {
        HideOverlay();
    }
}

void ScreenOverlay::CreateOverlayWindow() {
    // Register overlay window class
    WNDCLASS wc = {};
    wc.lpfnWndProc = OverlayWndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = OVERLAY_CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = NULL; // We'll handle painting ourselves
    
    RegisterClass(&wc);
    
    // Get screen dimensions
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    
    // Create fullscreen overlay window
    hOverlayWindow = CreateWindowEx(
        WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        OVERLAY_CLASS_NAME,
        "Overlay",
        WS_POPUP,
        0, 0, screenWidth, screenHeight,
        NULL, NULL, GetModuleHandle(NULL), this
    );
    
    if (hOverlayWindow) {
        // Set window as click-through for mouse events
        SetWindowLongPtr(hOverlayWindow, GWL_EXSTYLE, 
                        GetWindowLongPtr(hOverlayWindow, GWL_EXSTYLE) | WS_EX_TRANSPARENT);
    }
}

void ScreenOverlay::UpdateOverlayStyle() {
    if (!hOverlayWindow) return;
    
    switch (currentStyle) {
        case OVERLAY_BLUR:
            SetupBlurEffect();
            break;
        case OVERLAY_DIM:
            SetupDimEffect();
            break;
        case OVERLAY_BLACK:
            SetupBlackEffect();
            break;
        case OVERLAY_NONE:
        default:
            HideOverlay();
            break;
    }
}

void ScreenOverlay::SetupBlurEffect() {
    // Set semi-transparent background with blur hint
    SetLayeredWindowAttributes(hOverlayWindow, 0, 180, LWA_ALPHA);
    
    // Create a light gray brush for blur effect simulation
    if (hBackgroundBrush) DeleteObject(hBackgroundBrush);
    hBackgroundBrush = CreateSolidBrush(RGB(128, 128, 128));
    
    // Enable blur behind (Windows Aero effect)
    DWM_BLURBEHIND bb = {};
    bb.dwFlags = DWM_BB_ENABLE | DWM_BB_BLURREGION;
    bb.fEnable = TRUE;
    bb.hRgnBlur = CreateRectRgn(0, 0, -1, -1);
    DwmEnableBlurBehindWindow(hOverlayWindow, &bb);
    if (bb.hRgnBlur) DeleteObject(bb.hRgnBlur);
}

void ScreenOverlay::SetupDimEffect() {
    // Set semi-transparent dark background
    SetLayeredWindowAttributes(hOverlayWindow, 0, 120, LWA_ALPHA);
    
    // Create a dark brush
    if (hBackgroundBrush) DeleteObject(hBackgroundBrush);
    hBackgroundBrush = CreateSolidBrush(RGB(0, 0, 0));
    
    // Disable blur effect
    DWM_BLURBEHIND bb = {};
    bb.dwFlags = DWM_BB_ENABLE;
    bb.fEnable = FALSE;
    DwmEnableBlurBehindWindow(hOverlayWindow, &bb);
}

void ScreenOverlay::SetupBlackEffect() {
    // Set opaque black background
    SetLayeredWindowAttributes(hOverlayWindow, 0, 255, LWA_ALPHA);
    
    // Create a black brush
    if (hBackgroundBrush) DeleteObject(hBackgroundBrush);
    hBackgroundBrush = CreateSolidBrush(RGB(0, 0, 0));
    
    // Disable blur effect
    DWM_BLURBEHIND bb = {};
    bb.dwFlags = DWM_BB_ENABLE;
    bb.fEnable = FALSE;
    DwmEnableBlurBehindWindow(hOverlayWindow, &bb);
}

LRESULT CALLBACK ScreenOverlay::OverlayWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    ScreenOverlay* overlay = nullptr;
    
    if (uMsg == WM_CREATE) {
        CREATESTRUCT* cs = (CREATESTRUCT*)lParam;
        overlay = (ScreenOverlay*)cs->lpCreateParams;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)overlay);
    } else {
        overlay = (ScreenOverlay*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    }
    
    switch (uMsg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            
            if (overlay && overlay->hBackgroundBrush) {
                FillRect(hdc, &ps.rcPaint, overlay->hBackgroundBrush);
            }
            
            EndPaint(hwnd, &ps);
            return 0;
        }
        
        case WM_ERASEBKGND:
            return 1; // We handle background painting in WM_PAINT
            
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
            
        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    
    return 0;
}
