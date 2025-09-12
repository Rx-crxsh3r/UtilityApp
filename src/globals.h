// globals.h - Centralized extern declarations to avoid duplication
#ifndef GLOBALS_H
#define GLOBALS_H

#include <windows.h>
#include "features/productivity/productivity_manager.h"
#include "features/privacy/privacy_manager.h"
#include "features/lock_input/hotkey_manager.h"
#include "features/appearance/overlay_manager.h"
#include "features/lock_input/password_manager.h"
#include "features/lock_input/timer_manager.h"
#include "audio_manager.h"
#include "custom_notifications.h"

// Global window handle
extern HWND g_mainWindow;

// Global manager instances
extern ProductivityManager g_productivityManager;
extern PrivacyManager g_privacyManager;
extern HotkeyManager g_hotkeyManager;
extern OverlayManager g_overlayManager;
extern PasswordManager g_passwordManager;
extern TimerManager g_timerManager;
extern AudioManager* g_audioManager;
extern CustomNotifications* g_customNotifications;

// Global settings
extern AppSettings g_appSettings;

// Function declarations
void RegisterHotkeyFromSettings(HWND hwnd);

#endif // GLOBALS_H
