// src/notifications.h

#pragma once
#include <windows.h>

// Notification types
enum NotificationType {
    NOTIFY_APP_START,
    NOTIFY_APP_EXIT,
    NOTIFY_INPUT_LOCKED,
    NOTIFY_INPUT_UNLOCKED,
    NOTIFY_HOTKEY_ERROR,
    NOTIFY_FAILSAFE_TRIGGERED,
    NOTIFY_BOSS_KEY_ACTIVATED,
    NOTIFY_BOSS_KEY_DEACTIVATED,
    NOTIFY_USB_DEVICE_CONNECTED,
    NOTIFY_USB_DEVICE_DISCONNECTED,
    NOTIFY_QUICK_LAUNCH_EXECUTED,
    NOTIFY_WORK_SESSION_STARTED,
    NOTIFY_WORK_BREAK_STARTED,
    NOTIFY_SETTINGS_SAVED,
    NOTIFY_SETTINGS_LOADED,
    NOTIFY_SETTINGS_RESET,
    NOTIFY_SETTINGS_APPLIED
};

// Show a Windows toast notification
void ShowNotification(HWND hwnd, NotificationType type, const char* customMessage = nullptr);

// Show balloon tooltip in system tray
void ShowBalloonTip(HWND hwnd, const char* title, const char* message, DWORD iconType = NIIF_INFO);
