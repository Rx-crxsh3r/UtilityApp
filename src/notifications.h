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
    NOTIFY_FAILSAFE_TRIGGERED
};

// Show a Windows toast notification
void ShowNotification(HWND hwnd, NotificationType type, const char* customMessage = nullptr);

// Show balloon tooltip in system tray
void ShowBalloonTip(HWND hwnd, const char* title, const char* message, DWORD iconType = NIIF_INFO);
