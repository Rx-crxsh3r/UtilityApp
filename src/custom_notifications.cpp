// src/custom_notifications.cpp
// Custom lightweight notification system implementation

#include "custom_notifications.h"
#include <windows.h>
#include <dwmapi.h>

// Global instance
CustomNotificationSystem* g_customNotifications = nullptr;
CustomNotificationSystem* CustomNotificationSystem::instance = nullptr;

// External references
extern HWND g_mainWindow;

const char* NOTIFY_CLASS_NAME = "CustomNotifyClass";

CustomNotificationSystem::CustomNotificationSystem() 
    : hNotifyWindow(nullptr), hTitleFont(nullptr), hMessageFont(nullptr),
      hBackgroundBrush(nullptr), hBorderPen(nullptr), currentStyle(NOTIFY_STYLE_CUSTOM) {
    instance = this;
}

CustomNotificationSystem::~CustomNotificationSystem() {
    Cleanup();
    instance = nullptr;
}

CustomNotificationSystem* CustomNotificationSystem::GetInstance() {
    if (!instance) {
        instance = new CustomNotificationSystem();
    }
    return instance;
}

void CustomNotificationSystem::Initialize() {
    // Register window class
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = NotifyWndProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = NOTIFY_CLASS_NAME;
    wc.hbrBackground = nullptr; // We'll handle drawing
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    
    RegisterClassEx(&wc);
    
    CreateNotificationWindow();
    
    // Create fonts
    hTitleFont = CreateFont(-14, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
    
    hMessageFont = CreateFont(-12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
    
    // Create drawing objects
    hBackgroundBrush = CreateSolidBrush(BG_COLOR);
    hBorderPen = CreatePen(PS_SOLID, 1, RGB(40, 40, 40));
    
    // OPTIMIZATION: Reduce timer frequency to save CPU/RAM usage
    // 30 FPS is sufficient for notification animations and reduces resource usage
    SetTimer(hNotifyWindow, 1, 33, TimerProc); // ~30 FPS for balanced performance
}

void CustomNotificationSystem::CreateNotificationWindow() {
    hNotifyWindow = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED | WS_EX_TRANSPARENT,
        NOTIFY_CLASS_NAME,
        "",
        WS_POPUP,
        0, 0, 0, 0,
        nullptr, nullptr, GetModuleHandle(nullptr), this
    );
    
    // Enable per-pixel alpha for smooth transparency
    SetLayeredWindowAttributes(hNotifyWindow, 0, 255, LWA_ALPHA);
}

void CustomNotificationSystem::ShowNotification(const std::string& title, const std::string& message, DWORD duration) {
    if (currentStyle == NOTIFY_STYLE_NONE) return;
    
    if (currentStyle == NOTIFY_STYLE_WINDOWS) {
        // Use Windows native notifications
        MessageBox(g_mainWindow, message.c_str(), title.c_str(), MB_OK | MB_ICONINFORMATION | MB_TOPMOST);
        return;
    }
    
    if (currentStyle == NOTIFY_STYLE_WINDOWS_NOTIFICATIONS) {
        // Use Windows Action Center notifications (balloon tips)
        extern void ShowBalloonTip(HWND hwnd, const char* title, const char* message, DWORD iconType);
        extern HWND g_mainWindow;
        DWORD iconType = NIIF_INFO;
        ShowBalloonTip(g_mainWindow, title.c_str(), message.c_str(), iconType);
        return;
    }
    
    // NOTIFY_STYLE_CUSTOM - use custom notification system
    auto notif = std::make_unique<CustomNotification>(title, message, duration);
    
    // Position new notification
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    
    notif->targetY = screenHeight - NOTIFY_HEIGHT - NOTIFY_MARGIN - (notifications.size() * (NOTIFY_HEIGHT + 10));
    notif->yPosition = screenHeight; // Start off-screen
    
    notifications.push_back(std::move(notif));
    
    // Update window size and position
    PositionNotifications();
}

void CustomNotificationSystem::PositionNotifications() {
    if (notifications.empty()) {
        ShowWindow(hNotifyWindow, SW_HIDE);
        return;
    }
    
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    
    int totalHeight = notifications.size() * (NOTIFY_HEIGHT + 10) - 10;
    int windowX = screenWidth - NOTIFY_WIDTH - NOTIFY_MARGIN;
    int windowY = screenHeight - totalHeight - NOTIFY_MARGIN;
    
    SetWindowPos(hNotifyWindow, HWND_TOPMOST, 
                 windowX, windowY, 
                 NOTIFY_WIDTH, totalHeight,
                 SWP_NOACTIVATE | SWP_SHOWWINDOW);
}

void CustomNotificationSystem::UpdateNotifications() {
    DWORD currentTime = GetTickCount();
    bool needsUpdate = false;
    
    // Update notifications and remove expired ones
    for (auto it = notifications.begin(); it != notifications.end();) {
        auto& notif = *it;
        
        // Handle fading
        DWORD elapsed = currentTime - notif->showTime;
        if (elapsed < FADE_DURATION) {
            // Fade in
            notif->opacity = (float)elapsed / FADE_DURATION;
            needsUpdate = true;
        } else if (elapsed > notif->duration - FADE_DURATION) {
            // Fade out
            DWORD fadeElapsed = elapsed - (notif->duration - FADE_DURATION);
            notif->opacity = 1.0f - ((float)fadeElapsed / FADE_DURATION);
            needsUpdate = true;
            
            if (notif->opacity <= 0) {
                it = notifications.erase(it);
                continue;
            }
        } else {
            notif->opacity = 1.0f;
        }
        
        // Handle slide animation
        if (notif->yPosition != notif->targetY) {
            int diff = notif->targetY - notif->yPosition;
            notif->yPosition += diff / 8; // Smooth slide
            if (abs(diff) < 2) notif->yPosition = notif->targetY;
            needsUpdate = true;
        }
        
        ++it;
    }
    
    if (needsUpdate) {
        PositionNotifications();
        InvalidateRect(hNotifyWindow, nullptr, FALSE);
    }
}

void CustomNotificationSystem::DrawNotification(HDC hdc, CustomNotification* notif, int index) {
    int y = index * (NOTIFY_HEIGHT + 10);
    
    // Apply opacity
    BLENDFUNCTION blend = {};
    blend.BlendOp = AC_SRC_OVER;
    blend.BlendFlags = 0;
    blend.SourceConstantAlpha = (BYTE)(notif->opacity * 255);
    blend.AlphaFormat = 0;
    
    // Create memory DC for double buffering
    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP memBitmap = CreateCompatibleBitmap(hdc, NOTIFY_WIDTH, NOTIFY_HEIGHT);
    HBITMAP oldBitmap = (HBITMAP)SelectObject(memDC, memBitmap);
    
    // Draw background with rounded corners
    RECT rect = {0, 0, NOTIFY_WIDTH, NOTIFY_HEIGHT};
    FillRect(memDC, &rect, hBackgroundBrush);
    
    // Draw subtle border
    SelectObject(memDC, hBorderPen);
    SelectObject(memDC, GetStockObject(NULL_BRUSH));
    RoundRect(memDC, 0, 0, NOTIFY_WIDTH, NOTIFY_HEIGHT, 8, 8);
    
    // Draw text
    SetBkMode(memDC, TRANSPARENT);
    
    // Title
    SelectObject(memDC, hTitleFont);
    SetTextColor(memDC, TITLE_COLOR);
    RECT titleRect = {15, 10, NOTIFY_WIDTH - 15, 30};
    DrawText(memDC, notif->title.c_str(), -1, &titleRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    
    // Message
    SelectObject(memDC, hMessageFont);
    SetTextColor(memDC, TEXT_COLOR);
    RECT msgRect = {15, 32, NOTIFY_WIDTH - 15, NOTIFY_HEIGHT - 10};
    DrawText(memDC, notif->message.c_str(), -1, &msgRect, DT_LEFT | DT_TOP | DT_WORDBREAK | DT_END_ELLIPSIS);
    
    // Accent line at bottom (progress indicator)
    DWORD elapsed = GetTickCount() - notif->showTime;
    if (elapsed < notif->duration) {
        float progress = (float)elapsed / notif->duration;
        int lineWidth = (int)(NOTIFY_WIDTH * progress);
        
        HPEN accentPen = CreatePen(PS_SOLID, 2, ACCENT_COLOR);
        HPEN oldPen = (HPEN)SelectObject(memDC, accentPen);
        
        MoveToEx(memDC, 0, NOTIFY_HEIGHT - 2, nullptr);
        LineTo(memDC, lineWidth, NOTIFY_HEIGHT - 2);
        
        SelectObject(memDC, oldPen);
        DeleteObject(accentPen);
    }
    
    // Blend to main DC
    AlphaBlend(hdc, 0, y, NOTIFY_WIDTH, NOTIFY_HEIGHT,
               memDC, 0, 0, NOTIFY_WIDTH, NOTIFY_HEIGHT, blend);
    
    // Cleanup
    SelectObject(memDC, oldBitmap);
    DeleteObject(memBitmap);
    DeleteDC(memDC);
}

LRESULT CALLBACK CustomNotificationSystem::NotifyWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    CustomNotificationSystem* pThis = nullptr;
    
    if (msg == WM_CREATE) {
        CREATESTRUCT* pCreate = (CREATESTRUCT*)lParam;
        pThis = (CustomNotificationSystem*)pCreate->lpCreateParams;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);
    } else {
        pThis = (CustomNotificationSystem*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    }
    
    if (!pThis) return DefWindowProc(hwnd, msg, wParam, lParam);
    
    switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            
            // Clear background
            RECT rect;
            GetClientRect(hwnd, &rect);
            FillRect(hdc, &rect, (HBRUSH)GetStockObject(BLACK_BRUSH));
            
            // Draw all notifications
            for (size_t i = 0; i < pThis->notifications.size(); ++i) {
                pThis->DrawNotification(hdc, pThis->notifications[i].get(), (int)i);
            }
            
            EndPaint(hwnd, &ps);
            return 0;
        }
        
        case WM_LBUTTONDOWN: {
            // Click to dismiss
            pThis->ClearAll();
            return 0;
        }
        
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void CALLBACK CustomNotificationSystem::TimerProc(HWND hwnd, UINT msg, UINT_PTR idTimer, DWORD dwTime) {
    if (instance) {
        instance->UpdateNotifications();
    }
}

void CustomNotificationSystem::ClearAll() {
    notifications.clear();
    PositionNotifications();
}

void CustomNotificationSystem::Cleanup() {
    if (hNotifyWindow) {
        DestroyWindow(hNotifyWindow);
        hNotifyWindow = nullptr;
    }
    
    if (hTitleFont) {
        DeleteObject(hTitleFont);
        hTitleFont = nullptr;
    }
    
    if (hMessageFont) {
        DeleteObject(hMessageFont);
        hMessageFont = nullptr;
    }
    
    if (hBackgroundBrush) {
        DeleteObject(hBackgroundBrush);
        hBackgroundBrush = nullptr;
    }
    
    if (hBorderPen) {
        DeleteObject(hBorderPen);
        hBorderPen = nullptr;
    }
    
    UnregisterClass(NOTIFY_CLASS_NAME, GetModuleHandle(nullptr));
}

// Helper functions
void ShowCustomNotification(const std::string& title, const std::string& message) {
    if (g_customNotifications) {
        g_customNotifications->ShowNotification(title, message);
    }
}

void InitializeCustomNotifications() {
    if (!g_customNotifications) {
        g_customNotifications = CustomNotificationSystem::GetInstance();
        g_customNotifications->Initialize();
    }
}

void CleanupCustomNotifications() {
    if (g_customNotifications) {
        delete g_customNotifications;
        g_customNotifications = nullptr;
    }
}
