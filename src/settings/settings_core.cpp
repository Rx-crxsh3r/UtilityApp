// src/settings/settings_core.cpp
// Core settings management implementation

#include "settings_core.h"
#include "../notifications.h"
#include "../overlay.h"
#include "../custom_notifications.h"
#include "../features/appearance/overlay_manager.h"
#include "../features/lock_input/hotkey_manager.h"
#include "../features/privacy/privacy_manager.h"
#include "../features/productivity/productivity_manager.h"
#include <fstream>

// Global instance
SettingsCore g_settingsCore;

// Registry constants
const char* SettingsCore::REGISTRY_KEY = "SOFTWARE\\UtilityApp\\Core";

// Data integrity marker
const char* DATA_INTEGRITY_MARKER = "UtilityApp_Settings_v1.0";
const DWORD EXPECTED_SETTINGS_COUNT = 20; // Number of expected settings

// Export constants
const char* EXPORT_HEADER = "[UtilityApp Settings Export]\n";

// Validation constants
const int MIN_HOTKEY_VK = 0x08;
const int MAX_HOTKEY_VK = 0xFF;
const int MIN_TIMER_DURATION = 1;
const int MAX_TIMER_DURATION = 3600;
const int MAX_STRING_LENGTH = 100;

// Helper function for safe string to int conversion
int SafeStringToInt(const std::string& str, int defaultValue = 0) {
    try {
        return std::stoi(str);
    } catch (const std::invalid_argument&) {
        return defaultValue;
    } catch (const std::out_of_range&) {
        return defaultValue;
    }
}

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

    // First, validate data integrity
    std::string integrityMarker;
    DWORD settingsCount = 0;

    bool hasIntegrity = ReadRegistryString(hKey, "DataIntegrity", integrityMarker);
    ReadRegistryValue(hKey, "SettingsCount", settingsCount);

    if (!hasIntegrity || integrityMarker != DATA_INTEGRITY_MARKER || settingsCount != EXPECTED_SETTINGS_COUNT) {
        // Data is corrupted or from old version, use defaults
        RegCloseKey(hKey);
        ClearPersistentStorage(); // Clean up corrupted data
        settings = defaultSettings;
        return false;
    }

    // Data integrity validated, load all settings efficiently
    DWORD value;
    std::string strValue;
    int loadedSettings = 0;

    // Load DWORD values with validation
    if (ReadRegistryValue(hKey, "KeyboardLockEnabled", value)) {
        settings.keyboardLockEnabled = (value == 1);
        loadedSettings++;
    }
    if (ReadRegistryValue(hKey, "MouseLockEnabled", value)) {
        settings.mouseLockEnabled = (value == 1);
        loadedSettings++;
    }
    if (ReadRegistryValue(hKey, "UnlockMethod", value) && value <= 2) {
        settings.unlockMethod = value;
        loadedSettings++;
    }
    if (ReadRegistryValue(hKey, "EnableFailsafe", value)) {
        settings.enableFailsafe = (value == 1);
        loadedSettings++;
    }
    if (ReadRegistryValue(hKey, "HotkeyModifiers", value)) {
        settings.hotkeyModifiers = value;
        loadedSettings++;
    }
    if (ReadRegistryValue(hKey, "HotkeyVirtualKey", value) &&
        value >= MIN_HOTKEY_VK && value <= MAX_HOTKEY_VK) {
        settings.hotkeyVirtualKey = value;
        loadedSettings++;
    }
    if (ReadRegistryValue(hKey, "PasswordEnabled", value)) {
        settings.passwordEnabled = (value == 1);
        loadedSettings++;
    }
    if (ReadRegistryValue(hKey, "TimerDuration", value) &&
        value >= MIN_TIMER_DURATION && value <= MAX_TIMER_DURATION) {
        settings.timerDuration = value;
        loadedSettings++;
    }
    if (ReadRegistryValue(hKey, "TimerEnabled", value)) {
        settings.timerEnabled = (value == 1);
        loadedSettings++;
    }
    if (ReadRegistryValue(hKey, "WhitelistEnabled", value)) {
        settings.whitelistEnabled = (value == 1);
        loadedSettings++;
    }
    if (ReadRegistryValue(hKey, "OverlayStyle", value) && value <= 3) {
        settings.overlayStyle = value;
        loadedSettings++;
    }
    if (ReadRegistryValue(hKey, "NotificationStyle", value) && value <= 3) {
        settings.notificationStyle = value;
        loadedSettings++;
    }
    if (ReadRegistryValue(hKey, "HideFromTaskbar", value)) {
        settings.hideFromTaskbar = (value == 1);
        loadedSettings++;
    }
    if (ReadRegistryValue(hKey, "StartWithWindows", value)) {
        settings.startWithWindows = (value == 1);
        loadedSettings++;
    }
    if (ReadRegistryValue(hKey, "USBAlertEnabled", value)) {
        settings.usbAlertEnabled = (value == 1);
        loadedSettings++;
    }
    if (ReadRegistryValue(hKey, "QuickLaunchEnabled", value)) {
        settings.quickLaunchEnabled = (value == 1);
        loadedSettings++;
    }
    if (ReadRegistryValue(hKey, "WorkBreakTimerEnabled", value)) {
        settings.workBreakTimerEnabled = (value == 1);
        loadedSettings++;
    }
    if (ReadRegistryValue(hKey, "BossKeyEnabled", value)) {
        settings.bossKeyEnabled = (value == 1);
        loadedSettings++;
    }

    // Load string values with length validation
    if (ReadRegistryString(hKey, "LockHotkey", strValue) && strValue.length() <= MAX_STRING_LENGTH) {
        settings.lockHotkey = strValue;
        loadedSettings++;
    }
    if (ReadRegistryString(hKey, "UnlockPassword", strValue) && strValue.length() <= MAX_STRING_LENGTH) {
        settings.unlockPassword = strValue;
        loadedSettings++;
    }
    if (ReadRegistryString(hKey, "WhitelistedKeys", strValue) && strValue.length() <= MAX_STRING_LENGTH) {
        settings.whitelistedKeys = strValue;
        loadedSettings++;
    }
    if (ReadRegistryString(hKey, "BossKeyHotkey", strValue) && strValue.length() <= MAX_STRING_LENGTH) {
        settings.bossKeyHotkey = strValue;
        loadedSettings++;
    }

    RegCloseKey(hKey);

    // Validate that we loaded enough settings to consider data complete
    if (loadedSettings < (EXPECTED_SETTINGS_COUNT * 0.8)) { // At least 80% of settings
        // Insufficient valid data, reset to defaults
        ClearPersistentStorage();
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

    // Write data integrity marker first
    success &= WriteRegistryString(hKey, "DataIntegrity", DATA_INTEGRITY_MARKER);
    success &= WriteRegistryValue(hKey, "SettingsCount", EXPECTED_SETTINGS_COUNT);

    // Batch write all DWORD settings for efficiency
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

    // Write string values with length validation
    if (settings.lockHotkey.length() <= MAX_STRING_LENGTH) {
        success &= WriteRegistryString(hKey, "LockHotkey", settings.lockHotkey);
    }
    if (settings.unlockPassword.length() <= MAX_STRING_LENGTH) {
        success &= WriteRegistryString(hKey, "UnlockPassword", settings.unlockPassword);
    }
    if (settings.whitelistedKeys.length() <= MAX_STRING_LENGTH) {
        success &= WriteRegistryString(hKey, "WhitelistedKeys", settings.whitelistedKeys);
    }
    if (settings.bossKeyHotkey.length() <= MAX_STRING_LENGTH) {
        success &= WriteRegistryString(hKey, "BossKeyHotkey", settings.bossKeyHotkey);
    }

    RegCloseKey(hKey);

    // No backup system - focus on optimization and reliability
    return success;
}

bool SettingsCore::ClearPersistentStorage() {
    // Delete the entire registry key to simulate "no saved settings"
    // This ensures that on next app startup, LoadSettings() will fail and load defaults
    LONG result = RegDeleteKeyA(HKEY_CURRENT_USER, REGISTRY_KEY);
    return (result == ERROR_SUCCESS || result == ERROR_FILE_NOT_FOUND);
}

bool SettingsCore::ApplySettings(const AppSettings& settings, HWND mainWindow) {
    if (!ValidateSettings(settings)) {
        return false;
    }

    bool success = true;
    
    success &= ApplyHotkeySettings(settings);
    success &= ApplyPrivacySettings(settings, mainWindow);
    success &= ApplyProductivitySettings(settings, mainWindow);
    success &= ApplyOverlaySettings(settings);
    success &= ApplyNotificationSettings(settings);
    
    if (success && mainWindow) {
        ShowNotification(mainWindow, NOTIFY_SETTINGS_APPLIED);
    }
    
    return success;
}

bool SettingsCore::ApplySettings(const AppSettings& newSettings, const AppSettings& previousSettings, HWND mainWindow) {
    if (!ValidateSettings(newSettings)) {
        return false;
    }

    bool success = true;
    bool anyChangesApplied = false;
    
    // Only apply changed categories for performance optimization
    if (HasHotkeyChanges(newSettings, previousSettings)) {
        success &= ApplyHotkeySettings(newSettings);
        anyChangesApplied = true;
    }
    
    if (HasLockInputChanges(newSettings, previousSettings)) {
        // Lock input settings are applied by updating global variables in SettingsDialog
        anyChangesApplied = true;
    }
    
    if (HasPrivacyChanges(newSettings, previousSettings)) {
        success &= ApplyPrivacySettings(newSettings, mainWindow);
        anyChangesApplied = true;
    }
    
    if (HasProductivityChanges(newSettings, previousSettings)) {
        success &= ApplyProductivitySettings(newSettings, mainWindow);
        anyChangesApplied = true;
    }
    
    if (HasOverlayChanges(newSettings, previousSettings)) {
        success &= ApplyOverlaySettings(newSettings);
        anyChangesApplied = true;
    }
    
    if (HasNotificationChanges(newSettings, previousSettings)) {
        success &= ApplyNotificationSettings(newSettings);
        anyChangesApplied = true;
    }
    
    // Show notification only if changes were actually applied
    if (success && mainWindow && anyChangesApplied) {
        ShowNotification(mainWindow, NOTIFY_SETTINGS_APPLIED);
    } else if (!anyChangesApplied && mainWindow) {
        // Show a different message if no changes were detected
        ShowNotification(mainWindow, NOTIFY_SETTINGS_APPLIED, "No changes detected");
    }
    
    return success;
}

bool SettingsCore::ValidateSettings(const AppSettings& settings) {
    // Validate unlock method
    if (settings.unlockMethod < 0 || settings.unlockMethod > 2) {
        return false;
    }
    
    // Validate hotkey modifiers - must have at least one modifier for security
    if (settings.hotkeyModifiers == 0) {
        return false; // Must have at least one modifier (Ctrl, Alt, Shift, Win)
    }
    
    // Validate virtual key - must be a valid Windows virtual key code
    if (settings.hotkeyVirtualKey < MIN_HOTKEY_VK || settings.hotkeyVirtualKey > MAX_HOTKEY_VK) {
        return false;
    }
    
    // Validate overlay style
    if (settings.overlayStyle < 0 || settings.overlayStyle > 3) {
        return false;
    }
    
    // Validate notification style
    if (settings.notificationStyle < 0 || settings.notificationStyle > 3) {
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

bool SettingsCore::HasHotkeyChanges(const AppSettings& current, const AppSettings& original) {
    return current.lockHotkey != original.lockHotkey ||
           current.hotkeyModifiers != original.hotkeyModifiers ||
           current.hotkeyVirtualKey != original.hotkeyVirtualKey;
}

bool SettingsCore::HasLockInputChanges(const AppSettings& current, const AppSettings& original) {
    return current.keyboardLockEnabled != original.keyboardLockEnabled ||
           current.mouseLockEnabled != original.mouseLockEnabled ||
           current.unlockMethod != original.unlockMethod ||
           current.enableFailsafe != original.enableFailsafe ||
           current.whitelistEnabled != original.whitelistEnabled ||
           current.whitelistedKeys != original.whitelistedKeys ||
           current.unlockPassword != original.unlockPassword ||
           current.passwordEnabled != original.passwordEnabled ||
           current.timerDuration != original.timerDuration ||
           current.timerEnabled != original.timerEnabled;
}

bool SettingsCore::HasPrivacyChanges(const AppSettings& current, const AppSettings& original) {
    return current.hideFromTaskbar != original.hideFromTaskbar ||
           current.startWithWindows != original.startWithWindows ||
           current.bossKeyEnabled != original.bossKeyEnabled ||
           current.bossKeyHotkey != original.bossKeyHotkey;
}

bool SettingsCore::HasProductivityChanges(const AppSettings& current, const AppSettings& original) {
    return current.usbAlertEnabled != original.usbAlertEnabled ||
           current.quickLaunchEnabled != original.quickLaunchEnabled ||
           current.workBreakTimerEnabled != original.workBreakTimerEnabled;
}

bool SettingsCore::HasOverlayChanges(const AppSettings& current, const AppSettings& original) {
    return current.overlayStyle != original.overlayStyle;
}

bool SettingsCore::HasNotificationChanges(const AppSettings& current, const AppSettings& original) {
    return current.notificationStyle != original.notificationStyle;
}

bool SettingsCore::ExportToFile(const AppSettings& settings, const std::string& filepath) {
    std::ofstream file(filepath);
    if (!file.is_open()) {
        return false;
    }
    
    file << EXPORT_HEADER;
    // Lock & Input settings
    file << "KeyboardLockEnabled=" << (settings.keyboardLockEnabled ? 1 : 0) << "\n";
    file << "MouseLockEnabled=" << (settings.mouseLockEnabled ? 1 : 0) << "\n";
    file << "UnlockMethod=" << settings.unlockMethod << "\n";
    file << "EnableFailsafe=" << (settings.enableFailsafe ? 1 : 0) << "\n";
    file << "LockHotkey=" << settings.lockHotkey << "\n";
    
    // Hotkey settings
    file << "HotkeyModifiers=" << settings.hotkeyModifiers << "\n";
    file << "HotkeyVirtualKey=" << settings.hotkeyVirtualKey << "\n";
    
    // Password settings
    file << "UnlockPassword=" << settings.unlockPassword << "\n";
    file << "PasswordEnabled=" << (settings.passwordEnabled ? 1 : 0) << "\n";
    
    // Timer settings
    file << "TimerDuration=" << settings.timerDuration << "\n";
    file << "TimerEnabled=" << (settings.timerEnabled ? 1 : 0) << "\n";
    
    // Whitelist settings
    file << "WhitelistedKeys=" << settings.whitelistedKeys << "\n";
    file << "WhitelistEnabled=" << (settings.whitelistEnabled ? 1 : 0) << "\n";
    
    // Overlay settings
    file << "OverlayStyle=" << settings.overlayStyle << "\n";
    
    // Notification settings
    file << "NotificationStyle=" << settings.notificationStyle << "\n";
    
    // Privacy settings
    file << "HideFromTaskbar=" << (settings.hideFromTaskbar ? 1 : 0) << "\n";
    file << "StartWithWindows=" << (settings.startWithWindows ? 1 : 0) << "\n";
    
    // Productivity settings
    file << "USBAlertEnabled=" << (settings.usbAlertEnabled ? 1 : 0) << "\n";
    file << "QuickLaunchEnabled=" << (settings.quickLaunchEnabled ? 1 : 0) << "\n";
    file << "WorkBreakTimerEnabled=" << (settings.workBreakTimerEnabled ? 1 : 0) << "\n";
    file << "BossKeyEnabled=" << (settings.bossKeyEnabled ? 1 : 0) << "\n";
    file << "BossKeyHotkey=" << settings.bossKeyHotkey << "\n";
    
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
        
        // Handle integer values with safe conversion
        if (key == "KeyboardLockEnabled") newSettings.keyboardLockEnabled = (SafeStringToInt(value) != 0);
        else if (key == "MouseLockEnabled") newSettings.mouseLockEnabled = (SafeStringToInt(value) != 0);
        else if (key == "UnlockMethod") newSettings.unlockMethod = SafeStringToInt(value);
        else if (key == "EnableFailsafe") newSettings.enableFailsafe = (SafeStringToInt(value) != 0);
        else if (key == "HotkeyModifiers") newSettings.hotkeyModifiers = SafeStringToInt(value);
        else if (key == "HotkeyVirtualKey") newSettings.hotkeyVirtualKey = SafeStringToInt(value);
        else if (key == "PasswordEnabled") newSettings.passwordEnabled = (SafeStringToInt(value) != 0);
        else if (key == "TimerDuration") newSettings.timerDuration = SafeStringToInt(value);
        else if (key == "TimerEnabled") newSettings.timerEnabled = (SafeStringToInt(value) != 0);
        else if (key == "WhitelistEnabled") newSettings.whitelistEnabled = (SafeStringToInt(value) != 0);
        else if (key == "OverlayStyle") newSettings.overlayStyle = SafeStringToInt(value);
        else if (key == "NotificationStyle") newSettings.notificationStyle = SafeStringToInt(value);
        else if (key == "HideFromTaskbar") newSettings.hideFromTaskbar = (SafeStringToInt(value) != 0);
        else if (key == "StartWithWindows") newSettings.startWithWindows = (SafeStringToInt(value) != 0);
        else if (key == "USBAlertEnabled") newSettings.usbAlertEnabled = (SafeStringToInt(value) != 0);
        else if (key == "QuickLaunchEnabled") newSettings.quickLaunchEnabled = (SafeStringToInt(value) != 0);
        else if (key == "WorkBreakTimerEnabled") newSettings.workBreakTimerEnabled = (SafeStringToInt(value) != 0);
        else if (key == "BossKeyEnabled") newSettings.bossKeyEnabled = (SafeStringToInt(value) != 0);
        // Handle string values
        else if (key == "LockHotkey") newSettings.lockHotkey = value;
        else if (key == "UnlockPassword") newSettings.unlockPassword = value;
        else if (key == "WhitelistedKeys") newSettings.whitelistedKeys = value;
        else if (key == "BossKeyHotkey") newSettings.bossKeyHotkey = value;
    }
    
    file.close();
    
    // Use comprehensive validation for imported data
    if (ValidateImportedSettings(newSettings)) {
        settings = newSettings;
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

bool SettingsCore::WriteRegistryString(HKEY hKey, const char* valueName, const std::string& value) {
    return RegSetValueExA(hKey, valueName, 0, REG_SZ, (const BYTE*)value.c_str(), (DWORD)(value.length() + 1)) == ERROR_SUCCESS;
}

bool SettingsCore::ReadRegistryString(HKEY hKey, const char* valueName, std::string& value) {
    DWORD size = 0, type;
    if (RegQueryValueExA(hKey, valueName, NULL, &type, NULL, &size) != ERROR_SUCCESS || type != REG_SZ) {
        return false;
    }
    
    char* buffer = new char[size];
    bool success = RegQueryValueExA(hKey, valueName, NULL, &type, (BYTE*)buffer, &size) == ERROR_SUCCESS;
    if (success) {
        value = buffer;
    }
    delete[] buffer;
    return success;
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
    
    if (!mainWindow) {
        return false;
    }
    
    // Apply window privacy settings
    bool success = g_privacyManager.SetWindowPrivacy(mainWindow, 
                                                     settings.hideFromTaskbar);
    
    // Apply startup setting
    if (success) {
        bool startupResult = g_privacyManager.SetStartWithWindows(settings.startWithWindows);
        success &= startupResult;
    }
    
    // Apply boss key setting
    if (success) {
        if (settings.bossKeyEnabled) {
            // Parse boss key hotkey string to get modifiers and virtual key
            UINT modifiers, virtualKey;
            extern bool ParseHotkeyString(const std::string& hotkeyStr, UINT& modifiers, UINT& virtualKey);
            extern HotkeyManager g_hotkeyManager;
            
            if (ParseHotkeyString(settings.bossKeyHotkey, modifiers, virtualKey)) {
                // Check if the hotkey is available before trying to register
                if (g_hotkeyManager.IsHotkeyAvailable(modifiers, virtualKey)) {
                    bool bossKeyResult = g_privacyManager.SetBossKeyHotkey(modifiers, virtualKey);
                    // Don't fail the entire operation if boss key registration fails
                    // Boss key is secondary functionality - main settings should still apply
                    // success &= bossKeyResult;  // Commented out to prevent failure
                } else {
                    // Hotkey is not available, try fallback
                    if (g_hotkeyManager.IsHotkeyAvailable(MOD_CONTROL | MOD_ALT, VK_F11)) {
                        bool bossKeyResult = g_privacyManager.SetBossKeyHotkey(MOD_CONTROL | MOD_ALT, VK_F11);
                        // Don't fail the entire operation if boss key registration fails
                        // success &= bossKeyResult;  // Commented out to prevent failure
                    }
                    // If both primary and fallback fail, just continue without boss key
                }
            } else {
                // Fallback to default if parsing fails (Ctrl+Alt+F11)
                if (g_hotkeyManager.IsHotkeyAvailable(MOD_CONTROL | MOD_ALT, VK_F11)) {
                    bool bossKeyResult = g_privacyManager.SetBossKeyHotkey(MOD_CONTROL | MOD_ALT, VK_F11);
                    // Don't fail the entire operation if boss key registration fails
                    // success &= bossKeyResult;  // Commented out to prevent failure
                }
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

// Systematic layer management functions
void SettingsCore::UpdateAllLayers(const AppSettings& settings) {
    extern AppSettings g_appSettings;
    extern AppSettings g_persistentSettings;
    
    g_appSettings = settings;
    g_persistentSettings = settings;
}

bool SettingsCore::IsPersistentDataComplete() {
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, REGISTRY_KEY, 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
        return false; // No registry data at all
    }
    
    // Check if all critical settings exist
    DWORD value;
    std::string strValue;
    
    // Check critical DWORD values
    bool hasCriticalValues = 
        ReadRegistryValue(hKey, "KeyboardLockEnabled", value) &&
        ReadRegistryValue(hKey, "MouseLockEnabled", value) &&
        ReadRegistryValue(hKey, "UnlockMethod", value) &&
        ReadRegistryValue(hKey, "HotkeyModifiers", value) &&
        ReadRegistryValue(hKey, "HotkeyVirtualKey", value);
    
    // Check critical string values
    bool hasCriticalStrings =
        ReadRegistryString(hKey, "LockHotkey", strValue) &&
        ReadRegistryString(hKey, "UnlockPassword", strValue);
    
    RegCloseKey(hKey);
    
    return hasCriticalValues && hasCriticalStrings;
}

bool SettingsCore::ValidateImportedSettings(const AppSettings& settings) {
    // For imported data, we need to validate ranges and values
    if (settings.unlockMethod < 0 || settings.unlockMethod > 2) {
        return false;
    }
    
    if (settings.hotkeyModifiers == 0) {
        return false; // Must have at least one modifier
    }
    
    if (settings.hotkeyVirtualKey < MIN_HOTKEY_VK || settings.hotkeyVirtualKey > MAX_HOTKEY_VK) {
        return false;
    }
    
    if (settings.overlayStyle < 0 || settings.overlayStyle > 3) {
        return false;
    }
    
    if (settings.notificationStyle < 0 || settings.notificationStyle > 3) {
        return false;
    }
    
    if (settings.timerDuration < MIN_TIMER_DURATION || settings.timerDuration > MAX_TIMER_DURATION) {
        return false; // Reasonable timer range
    }
    
    // Validate string lengths
    if (settings.lockHotkey.length() > MAX_STRING_LENGTH || settings.unlockPassword.length() > MAX_STRING_LENGTH) {
        return false;
    }
    
    return true;
}
