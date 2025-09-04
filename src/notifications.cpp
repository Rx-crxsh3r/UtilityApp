// src/notifications.cpp

#include "notifications.h"
#include "custom_notifications.h"
#include "resource.h"
#include "settings.h"
#include <shellapi.h>

void ShowNotification(HWND hwnd, NotificationType type, const char* customMessage) {
    // Check notification style setting from appearance tab
    if (g_appSettings.notificationStyle == 3) { // NOTIFY_STYLE_NONE
        return; // No notifications
    }
    
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
            case NOTIFY_BOSS_KEY_ACTIVATED:
                message = "Boss Key activated - All windows hidden";
                iconType = NIIF_INFO;
                break;
            case NOTIFY_BOSS_KEY_DEACTIVATED:
                message = "Boss Key deactivated - Windows restored";
                iconType = NIIF_INFO;
                break;
            case NOTIFY_USB_DEVICE_CONNECTED:
                message = "USB device connected";
                iconType = NIIF_INFO;
                break;
            case NOTIFY_USB_DEVICE_DISCONNECTED:
                message = "USB device disconnected";
                iconType = NIIF_INFO;
                break;
            case NOTIFY_QUICK_LAUNCH_EXECUTED:
                message = "Quick launch application executed";
                iconType = NIIF_INFO;
                break;
            case NOTIFY_WORK_SESSION_STARTED:
                message = "Work session started";
                iconType = NIIF_INFO;
                break;
            case NOTIFY_WORK_BREAK_STARTED:
                message = "Break time started";
                iconType = NIIF_INFO;
                break;
            case NOTIFY_SETTINGS_SAVED:
                message = "Settings saved successfully";
                iconType = NIIF_INFO;
                break;
            case NOTIFY_SETTINGS_LOADED:
                message = "Settings loaded successfully";
                iconType = NIIF_INFO;
                break;
            case NOTIFY_SETTINGS_RESET:
                message = "Settings reset to defaults";
                iconType = NIIF_INFO;
                break;
            case NOTIFY_SETTINGS_APPLIED:
                message = "All settings have been successfully applied";
                iconType = NIIF_INFO;
                break;
            default:
                message = "Unknown notification";
                break;
        }
    }
    
    if (g_appSettings.notificationStyle == 1) { // NOTIFY_STYLE_WINDOWS (MessageBox)
        // Use Windows native message boxes
        MessageBoxA(NULL, message, title, MB_OK | 
                   (iconType == NIIF_ERROR ? MB_ICONERROR : 
                    iconType == NIIF_WARNING ? MB_ICONWARNING : MB_ICONINFORMATION) | 
                   MB_TOPMOST | MB_SYSTEMMODAL);
        return;
    }
    
    if (g_appSettings.notificationStyle == 2) { // NOTIFY_STYLE_WINDOWS_NOTIFICATIONS
        // Use Windows Action Center notifications (balloon tips)
        ShowBalloonTip(hwnd, title, message, iconType);
        return;
    }
    
    // NOTIFY_STYLE_CUSTOM (default) - use custom notification system
    // OPTIMIZATION: Defer notification display to avoid interfering with input processing
    // This prevents notifications from causing input lag during password entry
    PostMessage(hwnd, WM_USER + 102, (WPARAM)type, (LPARAM)_strdup(message));
}

void ShowBalloonTip(HWND hwnd, const char* title, const char* message, DWORD iconType) {
    NOTIFYICONDATAA nid = {};
    nid.cbSize = sizeof(NOTIFYICONDATAA);
    nid.hWnd = hwnd;
    nid.uID = 1;
    nid.uFlags = NIF_INFO; // Only NIF_INFO for balloon tip
    nid.dwInfoFlags = iconType;
    nid.uTimeout = 4000; // 4 seconds
    
    // Copy strings safely
    strncpy_s(nid.szInfoTitle, sizeof(nid.szInfoTitle), title, _TRUNCATE);
    strncpy_s(nid.szInfo, sizeof(nid.szInfo), message, _TRUNCATE);
    
    Shell_NotifyIconA(NIM_MODIFY, &nid);
}
