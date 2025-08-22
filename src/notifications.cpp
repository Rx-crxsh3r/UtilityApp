// src/notifications.cpp

#include "notifications.h"
#include "custom_notifications.h"
#include "resource.h"
#include <shellapi.h>

void ShowNotification(HWND hwnd, NotificationType type, const char* customMessage) {
    const char* title = "UtilityApp";
    const char* message = customMessage;
    DWORD iconType = NIIF_INFO;
    
    if (!customMessage) {
        switch (type) {
            case NOTIFY_APP_START:
                message = "Application started and running in background";
                iconType = NIIF_INFO;
                break;
            case NOTIFY_APP_EXIT:
                message = "Application is shutting down";
                iconType = NIIF_INFO;
                break;
            case NOTIFY_INPUT_LOCKED:
                message = "Keyboard and mouse input has been LOCKED";
                iconType = NIIF_WARNING;
                break;
            case NOTIFY_INPUT_UNLOCKED:
                message = "Keyboard and mouse input has been UNLOCKED";
                iconType = NIIF_INFO;
                break;
            case NOTIFY_HOTKEY_ERROR:
                message = "Failed to register hotkeys";
                iconType = NIIF_ERROR;
                break;
            case NOTIFY_FAILSAFE_TRIGGERED:
                message = "Failsafe triggered - Application shutting down";
                iconType = NIIF_WARNING;
                break;
            default:
                message = "Unknown notification";
                break;
        }
    }
    
    // Use custom notification system if available
    if (g_customNotifications) {
        g_customNotifications->ShowNotification(title, message);
    } else {
        // Fallback to system tray balloon
        ShowBalloonTip(hwnd, title, message, iconType);
    }
}

void ShowBalloonTip(HWND hwnd, const char* title, const char* message, DWORD iconType) {
    NOTIFYICONDATAA nid = {};
    nid.cbSize = sizeof(NOTIFYICONDATAA);
    nid.hWnd = hwnd;
    nid.uID = 1;
    nid.uFlags = NIF_INFO;
    nid.dwInfoFlags = iconType;
    
    // Copy strings safely
    strncpy_s(nid.szInfoTitle, sizeof(nid.szInfoTitle), title, _TRUNCATE);
    strncpy_s(nid.szInfo, sizeof(nid.szInfo), message, _TRUNCATE);
    
    Shell_NotifyIconA(NIM_MODIFY, &nid);
}
