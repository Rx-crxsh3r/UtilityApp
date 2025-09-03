// src/settings/settings_core.cpp
// Core settings management implementation

#include "settings_core.h"
#include "../notifications.h"
#include "../overlay.h"
#include "../custom_notifications.h"
#include "overlay_manager.h"
#include "hotkey_manager.h"
#include "../features/privacy/privacy_manager.h"
#include "../features/productivity/productivity_manager.h"
#include <fstream>
#include <sstream>

// Global instance
SettingsCore g_settingsCore;

// Registry constants
const char* SettingsCore::REGISTRY_KEY = "SOFTWARE\\UtilityApp\\Core";

SettingsCore::SettingsCore() {
    // Initialize default settings
    defaultSettings = AppSettings();
}

SettingsCore::~SettingsCore() {
    // Cleanup if needed
}

bool SettingsCore::LoadSettings(AppSettings& settings) {
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, REGISTRY_KEY, 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
        // No saved settings, use defaults
        settings = defaultSettings;
        return false;
    }

    DWORD value;
    
    // Load all settings values
    if (ReadRegistryValue(hKey, "KeyboardLockEnabled", value)) {
        settings.keyboardLockEnabled = (value != 0);
    }
    
    if (ReadRegistryValue(hKey, "MouseLockEnabled", value)) {
        settings.mouseLockEnabled = (value != 0);
    }
    
    if (ReadRegistryValue(hKey, "UnlockMethod", value)) {
        settings.unlockMethod = value;
    }
    
    if (ReadRegistryValue(hKey, "EnableFailsafe", value)) {
        settings.enableFailsafe = (value != 0);
    }
    
    if (ReadRegistryValue(hKey, "HotkeyModifiers", value)) {
        settings.hotkeyModifiers = value;
    }
    
    if (ReadRegistryValue(hKey, "HotkeyVirtualKey", value)) {
        settings.hotkeyVirtualKey = value;
    }
    
    if (ReadRegistryValue(hKey, "PasswordEnabled", value)) {
        settings.passwordEnabled = (value != 0);
    }
    
    if (ReadRegistryValue(hKey, "TimerDuration", value)) {
        settings.timerDuration = value;
    }
    
    if (ReadRegistryValue(hKey, "TimerEnabled", value)) {
        settings.timerEnabled = (value != 0);
    }
    
    if (ReadRegistryValue(hKey, "WhitelistEnabled", value)) {
        settings.whitelistEnabled = (value != 0);
    }
    
    if (ReadRegistryValue(hKey, "OverlayStyle", value)) {
        settings.overlayStyle = value;
    }
    
    if (ReadRegistryValue(hKey, "NotificationStyle", value)) {
        settings.notificationStyle = value;
    }
    
    if (ReadRegistryValue(hKey, "HideFromTaskbar", value)) {
        settings.hideFromTaskbar = (value != 0);
    }
    
    if (ReadRegistryValue(hKey, "StartWithWindows", value)) {
        settings.startWithWindows = (value != 0);
    }
    
    // Productivity settings
    if (ReadRegistryValue(hKey, "USBAlertEnabled", value)) {
        settings.usbAlertEnabled = (value != 0);
    }
    
    if (ReadRegistryValue(hKey, "QuickLaunchEnabled", value)) {
        settings.quickLaunchEnabled = (value != 0);
    }
    
    if (ReadRegistryValue(hKey, "WorkBreakTimerEnabled", value)) {
        settings.workBreakTimerEnabled = (value != 0);
    }
    
    if (ReadRegistryValue(hKey, "BossKeyEnabled", value)) {
        settings.bossKeyEnabled = (value != 0);
    }

    RegCloseKey(hKey);
    
    // Validate loaded settings
    if (!ValidateSettings(settings)) {
        settings = defaultSettings;
        return false;
    }
    
    return true;
}

bool SettingsCore::SaveSettings(const AppSettings& settings) {
    if (!ValidateSettings(settings)) {
        return false;
    }

    HKEY hKey;
    if (RegCreateKeyExA(HKEY_CURRENT_USER, REGISTRY_KEY, 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) != ERROR_SUCCESS) {
        return false;
    }

    bool success = true;
    
    // Save all settings values
    success &= WriteRegistryValue(hKey, "KeyboardLockEnabled", settings.keyboardLockEnabled ? 1 : 0);
    success &= WriteRegistryValue(hKey, "MouseLockEnabled", settings.mouseLockEnabled ? 1 : 0);
    success &= WriteRegistryValue(hKey, "UnlockMethod", settings.unlockMethod);
    success &= WriteRegistryValue(hKey, "EnableFailsafe", settings.enableFailsafe ? 1 : 0);
    success &= WriteRegistryValue(hKey, "HotkeyModifiers", settings.hotkeyModifiers);
    success &= WriteRegistryValue(hKey, "HotkeyVirtualKey", settings.hotkeyVirtualKey);
    success &= WriteRegistryValue(hKey, "PasswordEnabled", settings.passwordEnabled ? 1 : 0);
    success &= WriteRegistryValue(hKey, "TimerDuration", settings.timerDuration);
    success &= WriteRegistryValue(hKey, "TimerEnabled", settings.timerEnabled ? 1 : 0);
    success &= WriteRegistryValue(hKey, "WhitelistEnabled", settings.whitelistEnabled ? 1 : 0);
    success &= WriteRegistryValue(hKey, "OverlayStyle", settings.overlayStyle);
    success &= WriteRegistryValue(hKey, "NotificationStyle", settings.notificationStyle);
    success &= WriteRegistryValue(hKey, "HideFromTaskbar", settings.hideFromTaskbar ? 1 : 0);
    success &= WriteRegistryValue(hKey, "StartWithWindows", settings.startWithWindows ? 1 : 0);
    
    // Productivity settings
    success &= WriteRegistryValue(hKey, "USBAlertEnabled", settings.usbAlertEnabled ? 1 : 0);
    success &= WriteRegistryValue(hKey, "QuickLaunchEnabled", settings.quickLaunchEnabled ? 1 : 0);
    success &= WriteRegistryValue(hKey, "WorkBreakTimerEnabled", settings.workBreakTimerEnabled ? 1 : 0);
    success &= WriteRegistryValue(hKey, "BossKeyEnabled", settings.bossKeyEnabled ? 1 : 0);

    RegCloseKey(hKey);
    
    if (success) {
        CreateBackup(settings); // Auto-backup on successful save
    }
    
    return success;
}

bool SettingsCore::ApplySettings(const AppSettings& settings, HWND mainWindow) {
    if (!ValidateSettings(settings)) {
        return false;
    }

    bool success = true;
    
    // Apply different categories of settings
    success &= ApplyHotkeySettings(settings);
    success &= ApplyPrivacySettings(settings, mainWindow);
    success &= ApplyProductivitySettings(settings, mainWindow);
    success &= ApplyOverlaySettings(settings);
    success &= ApplyNotificationSettings(settings);
    
    if (success && mainWindow) {
        ShowNotification(mainWindow, NOTIFY_INPUT_UNLOCKED, "All settings have been successfully applied.");
    }
    
    return success;
}

bool SettingsCore::ValidateSettings(const AppSettings& settings) {
    // Validate unlock method
    if (settings.unlockMethod < 0 || settings.unlockMethod > 2) {
        return false;
    }
    
    // Validate hotkey modifiers
    if (settings.hotkeyModifiers == 0) {
        return false; // Must have at least one modifier
    }
    
    // Validate virtual key
    if (settings.hotkeyVirtualKey < 0x08 || settings.hotkeyVirtualKey > 0xFF) {
        return false;
    }
    
    // Validate overlay style
    if (settings.overlayStyle < 0 || settings.overlayStyle > 3) {
        return false;
    }
    
    // Validate notification style
    if (settings.notificationStyle < 0 || settings.notificationStyle > 2) {
        return false;
    }
    
    return true;
}

void SettingsCore::ResetToDefaults(AppSettings& settings) {
    settings = defaultSettings;
}

bool SettingsCore::HasChanges(const AppSettings& current, const AppSettings& original) {
    return current != original;
}

bool SettingsCore::ExportToFile(const AppSettings& settings, const std::string& filepath) {
    std::ofstream file(filepath);
    if (!file.is_open()) {
        return false;
    }
    
    file << "[UtilityApp Settings Export]\n";
    file << "UnlockMethod=" << settings.unlockMethod << "\n";
    file << "EnableFailsafe=" << (settings.enableFailsafe ? 1 : 0) << "\n";
    file << "HotkeyModifiers=" << settings.hotkeyModifiers << "\n";
    file << "HotkeyVirtualKey=" << settings.hotkeyVirtualKey << "\n";
    file << "OverlayStyle=" << settings.overlayStyle << "\n";
    file << "NotificationStyle=" << settings.notificationStyle << "\n";
    file << "HideFromTaskbar=" << (settings.hideFromTaskbar ? 1 : 0) << "\n";
    file << "StartWithWindows=" << (settings.startWithWindows ? 1 : 0) << "\n";
    
    file.close();
    return true;
}

bool SettingsCore::ImportFromFile(AppSettings& settings, const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        return false;
    }
    
    AppSettings newSettings = defaultSettings;
    std::string line;
    
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '[' || line[0] == '#') continue;
        
        size_t pos = line.find('=');
        if (pos == std::string::npos) continue;
        
        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1);
        
        int intValue = std::stoi(value);
        
        if (key == "UnlockMethod") newSettings.unlockMethod = intValue;
        else if (key == "EnableFailsafe") newSettings.enableFailsafe = (intValue != 0);
        else if (key == "HotkeyModifiers") newSettings.hotkeyModifiers = intValue;
        else if (key == "HotkeyVirtualKey") newSettings.hotkeyVirtualKey = intValue;
        else if (key == "OverlayStyle") newSettings.overlayStyle = intValue;
        else if (key == "NotificationStyle") newSettings.notificationStyle = intValue;
        else if (key == "HideFromTaskbar") newSettings.hideFromTaskbar = (intValue != 0);
        else if (key == "StartWithWindows") newSettings.startWithWindows = (intValue != 0);
    }
    
    file.close();
    
    if (ValidateSettings(newSettings)) {
        settings = newSettings;
        return true;
    }
    
    return false;
}

bool SettingsCore::CreateBackup(const AppSettings& settings) {
    // Create backup in registry under different key
    HKEY hKey;
    const char* backupKey = "SOFTWARE\\UtilityApp\\Backup";
    
    if (RegCreateKeyExA(HKEY_CURRENT_USER, backupKey, 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) != ERROR_SUCCESS) {
        return false;
    }

    bool success = true;
    success &= WriteRegistryValue(hKey, "UnlockMethod", settings.unlockMethod);
    success &= WriteRegistryValue(hKey, "EnableFailsafe", settings.enableFailsafe ? 1 : 0);
    success &= WriteRegistryValue(hKey, "HotkeyModifiers", settings.hotkeyModifiers);
    success &= WriteRegistryValue(hKey, "HotkeyVirtualKey", settings.hotkeyVirtualKey);
    success &= WriteRegistryValue(hKey, "OverlayStyle", settings.overlayStyle);
    success &= WriteRegistryValue(hKey, "NotificationStyle", settings.notificationStyle);
    success &= WriteRegistryValue(hKey, "HideFromTaskbar", settings.hideFromTaskbar ? 1 : 0);
    success &= WriteRegistryValue(hKey, "StartWithWindows", settings.startWithWindows ? 1 : 0);
    
    // Productivity settings
    success &= WriteRegistryValue(hKey, "USBAlertEnabled", settings.usbAlertEnabled ? 1 : 0);
    success &= WriteRegistryValue(hKey, "QuickLaunchEnabled", settings.quickLaunchEnabled ? 1 : 0);
    success &= WriteRegistryValue(hKey, "WorkBreakTimerEnabled", settings.workBreakTimerEnabled ? 1 : 0);
    success &= WriteRegistryValue(hKey, "BossKeyEnabled", settings.bossKeyEnabled ? 1 : 0);

    RegCloseKey(hKey);
    return success;
}

bool SettingsCore::RestoreFromBackup(AppSettings& settings) {
    HKEY hKey;
    const char* backupKey = "SOFTWARE\\UtilityApp\\Backup";
    
    if (RegOpenKeyExA(HKEY_CURRENT_USER, backupKey, 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
        return false;
    }

    AppSettings backupSettings = defaultSettings;
    DWORD value;
    
    if (ReadRegistryValue(hKey, "UnlockMethod", value)) backupSettings.unlockMethod = value;
    if (ReadRegistryValue(hKey, "EnableFailsafe", value)) backupSettings.enableFailsafe = (value != 0);
    if (ReadRegistryValue(hKey, "HotkeyModifiers", value)) backupSettings.hotkeyModifiers = value;
    if (ReadRegistryValue(hKey, "HotkeyVirtualKey", value)) backupSettings.hotkeyVirtualKey = value;
    if (ReadRegistryValue(hKey, "OverlayStyle", value)) backupSettings.overlayStyle = value;
    if (ReadRegistryValue(hKey, "NotificationStyle", value)) backupSettings.notificationStyle = value;
    if (ReadRegistryValue(hKey, "HideFromTaskbar", value)) backupSettings.hideFromTaskbar = (value != 0);
    if (ReadRegistryValue(hKey, "StartWithWindows", value)) backupSettings.startWithWindows = (value != 0);

    RegCloseKey(hKey);
    
    if (ValidateSettings(backupSettings)) {
        settings = backupSettings;
        return true;
    }
    
    return false;
}

bool SettingsCore::WriteRegistryValue(HKEY hKey, const char* valueName, DWORD value) {
    return RegSetValueExA(hKey, valueName, 0, REG_DWORD, (const BYTE*)&value, sizeof(DWORD)) == ERROR_SUCCESS;
}

bool SettingsCore::ReadRegistryValue(HKEY hKey, const char* valueName, DWORD& value) {
    DWORD size = sizeof(DWORD), type;
    return RegQueryValueExA(hKey, valueName, NULL, &type, (BYTE*)&value, &size) == ERROR_SUCCESS && type == REG_DWORD;
}

bool SettingsCore::ApplyHotkeySettings(const AppSettings& settings) {
    // Apply hotkey settings by re-registering all hotkeys
    extern HWND g_mainWindow;
    extern void RegisterHotkeyFromSettings(HWND hwnd);
    
    if (g_mainWindow) {
        RegisterHotkeyFromSettings(g_mainWindow);
        return true;
    }
    
    return false;
}

bool SettingsCore::ApplyPrivacySettings(const AppSettings& settings, HWND mainWindow) {
    // Apply privacy settings using the dedicated privacy manager
    extern PrivacyManager g_privacyManager;
    
    if (!mainWindow) return false;
    
    // Apply window privacy settings
    bool success = g_privacyManager.SetWindowPrivacy(mainWindow, 
                                                     settings.hideFromTaskbar);
    
    // Apply startup setting
    if (success) {
        success &= g_privacyManager.SetStartWithWindows(settings.startWithWindows);
    }
    
    // Apply boss key setting
    if (success) {
        if (settings.bossKeyEnabled) {
            // Parse boss key hotkey string to get modifiers and virtual key
            UINT modifiers, virtualKey;
            extern bool ParseHotkeyString(const std::string& hotkeyStr, UINT& modifiers, UINT& virtualKey);
            if (ParseHotkeyString(settings.bossKeyHotkey, modifiers, virtualKey)) {
                success &= g_privacyManager.EnableBossKey(modifiers, virtualKey);
            } else {
                // Fallback to default if parsing fails (Ctrl+Alt+F11)
                success &= g_privacyManager.EnableBossKey(MOD_CONTROL | MOD_ALT, VK_F11);
            }
        } else {
            g_privacyManager.DisableBossKey();
        }
    }
    
    return success;
}

bool SettingsCore::ApplyProductivitySettings(const AppSettings& settings, HWND mainWindow) {
    // Apply productivity settings using the dedicated productivity manager
    extern ProductivityManager g_productivityManager;
    
    if (!mainWindow) return false;
    
    bool success = true;
    
    // Apply USB alert setting
    if (settings.usbAlertEnabled) {
        g_productivityManager.EnableUSBAlert(mainWindow);
    } else {
        g_productivityManager.DisableUSBAlert();
    }
    
    // Apply quick launch setting
    if (settings.quickLaunchEnabled) {
        g_productivityManager.EnableQuickLaunch();
    } else {
        g_productivityManager.DisableQuickLaunch();
    }
    
    // Apply work/break timer setting
    if (settings.workBreakTimerEnabled) {
        g_productivityManager.EnableWorkBreakTimer(mainWindow);
    } else {
        g_productivityManager.DisableWorkBreakTimer();
    }
    
    return success;
}

bool SettingsCore::ApplyOverlaySettings(const AppSettings& settings) {
    // Apply overlay settings to the global overlay manager
    extern OverlayManager g_overlayManager;
    extern ScreenOverlay g_screenOverlay;
    
    // Update overlay manager with new style
    g_overlayManager.SetStyle((OverlayStyle)settings.overlayStyle);
    
    // Update the screen overlay system
    g_screenOverlay.SetStyle((OverlayStyle)settings.overlayStyle);
    
    return true;
}

bool SettingsCore::ApplyNotificationSettings(const AppSettings& settings) {
    // Apply notification settings to the custom notification system
    if (g_customNotifications) {
        g_customNotifications->SetStyle((NotificationStyle)settings.notificationStyle);
    }
    
    return true;
}
