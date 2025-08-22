// src/custom_notifications.h
// Custom lightweight notification system

#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include <memory>

enum NotificationStyle {
    NOTIFY_STYLE_CUSTOM = 0,    // Our custom black popup
    NOTIFY_STYLE_WINDOWS = 1,   // Windows native notifications
    NOTIFY_STYLE_NONE = 2       // No notifications
};

struct CustomNotification {
    std::string title;
    std::string message;
    DWORD showTime;
    DWORD duration;
    bool isVisible;
    float opacity;
    int yPosition;
    int targetY;
    
    CustomNotification(const std::string& t, const std::string& m, DWORD dur = 4000) 
        : title(t), message(m), showTime(GetTickCount()), duration(dur), 
          isVisible(true), opacity(0.0f), yPosition(0), targetY(0) {}
};

class CustomNotificationSystem {
private:
    static CustomNotificationSystem* instance;
    
    HWND hNotifyWindow;
    std::vector<std::unique_ptr<CustomNotification>> notifications;
    HFONT hTitleFont;
    HFONT hMessageFont;
    HBRUSH hBackgroundBrush;
    HPEN hBorderPen;
    
    // Notification properties
    static const int NOTIFY_WIDTH = 320;
    static const int NOTIFY_HEIGHT = 80;
    static const int NOTIFY_MARGIN = 10;
    static const int FADE_DURATION = 200;
    
    // Colors
    static const COLORREF BG_COLOR = RGB(13, 13, 13);      // #0D0D0D
    static const COLORREF TEXT_COLOR = RGB(221, 221, 221);  // #DDDDDD
    static const COLORREF TITLE_COLOR = RGB(255, 255, 255); // #FFFFFF
    static const COLORREF ACCENT_COLOR = RGB(58, 159, 255); // #3A9FFF
    
    void CreateNotificationWindow();
    void UpdateNotifications();
    void DrawNotification(HDC hdc, CustomNotification* notif, int index);
    void PositionNotifications();
    
    static LRESULT CALLBACK NotifyWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static void CALLBACK TimerProc(HWND hwnd, UINT msg, UINT_PTR idTimer, DWORD dwTime);

public:
    CustomNotificationSystem();
    ~CustomNotificationSystem();
    
    static CustomNotificationSystem* GetInstance();
    
    void Initialize();
    void Cleanup();
    void ShowNotification(const std::string& title, const std::string& message, DWORD duration = 4000);
    void ClearAll();
    void SetStyle(NotificationStyle style) { currentStyle = style; }
    NotificationStyle GetStyle() const { return currentStyle; }
    
private:
    NotificationStyle currentStyle;
};

// Global instance
extern CustomNotificationSystem* g_customNotifications;

// Helper functions
void ShowCustomNotification(const std::string& title, const std::string& message);
void InitializeCustomNotifications();
void CleanupCustomNotifications();
