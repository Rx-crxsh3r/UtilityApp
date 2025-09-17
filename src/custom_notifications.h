// src/custom_notifications.h
// Custom lightweight notification system

#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include <memory>

enum NotificationStyle {
    NOTIFY_STYLE_CUSTOM = 0,               // Our custom black popup
    NOTIFY_STYLE_WINDOWS = 1,              // Windows message boxes
    NOTIFY_STYLE_WINDOWS_NOTIFICATIONS = 2, // Windows Action Center notifications
    NOTIFY_STYLE_NONE = 3                  // No notifications
};

enum NotificationLevel {
    NOTIFY_LEVEL_INFO = 0,
    NOTIFY_LEVEL_WARNING = 1,
    NOTIFY_LEVEL_ERROR = 2
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
    NotificationLevel level;
    
    CustomNotification(const std::string& t, const std::string& m, DWORD dur = 4000, NotificationLevel lvl = NOTIFY_LEVEL_INFO) 
        : title(t), message(m), showTime(GetTickCount()), duration(dur), 
          isVisible(true), opacity(0.0f), yPosition(0), targetY(0), level(lvl) {}
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
    
    // Error notification colors
    static const COLORREF ERROR_BG_COLOR = RGB(40, 13, 13);    // Dark red background
    static const COLORREF ERROR_ACCENT_COLOR = RGB(255, 58, 58); // Red accent
    static const COLORREF ERROR_BORDER_COLOR = RGB(180, 40, 40); // Red border
    
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
    void ShowNotification(const std::string& title, const std::string& message, DWORD duration = 4000, NotificationLevel level = NOTIFY_LEVEL_INFO);
    void ClearAll();
    void SetStyle(NotificationStyle style) { currentStyle = style; }
    NotificationStyle GetStyle() const { return currentStyle; }
    
private:
    NotificationStyle currentStyle;
};

// Global instance
extern CustomNotificationSystem* g_customNotifications;

// Helper functions
void ShowCustomNotification(const std::string& title, const std::string& message, NotificationLevel level = NOTIFY_LEVEL_INFO);
void InitializeCustomNotifications();
void CleanupCustomNotifications();
