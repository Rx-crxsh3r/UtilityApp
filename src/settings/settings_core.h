// src/settings/settings_core.h
// Core settings management, load/save/apply functionality

#pragma once
#include <windows.h>
#include <string>

// Settings structure (comprehensive for all app functionality)
struct AppSettings {
    // Lock & Input
    bool keyboardLockEnabled;
    bool mouseLockEnabled;
    int unlockMethod;      // 0=password, 1=timer, 2=whitelist
    bool enableFailsafe;
    std::string lockHotkey;
    
    // Hotkey
    int hotkeyModifiers;   // Combination of MOD_CONTROL, MOD_SHIFT, etc.
    int hotkeyVirtualKey;  // Virtual key code
    
    // Password settings
    std::string unlockPassword;
    bool passwordEnabled;
    
    // Timer settings  
    int timerDuration; // seconds
    bool timerEnabled;
    
    // Whitelist settings
    std::string whitelistedKeys;
    bool whitelistEnabled;
    
    // Overlay
    int overlayStyle;      // 0=blur, 1=dim, 2=black, 3=none
    
    // Notifications  
    int notificationStyle; // 0=custom, 1=windows, 2=windows_notifications, 3=none
    
    // Privacy
    bool hideFromTaskbar;
    bool startWithWindows;
    
    // Productivity  
    bool usbAlertEnabled;
    bool quickLaunchEnabled;
    bool workBreakTimerEnabled;
    bool bossKeyEnabled;
    std::string bossKeyHotkey;
    
    // Default constructor
    AppSettings() {
        keyboardLockEnabled = true;
        mouseLockEnabled = true;
        unlockMethod = 0;
        enableFailsafe = true;
        lockHotkey = "Ctrl+Shift+L";
        hotkeyModifiers = MOD_CONTROL | MOD_SHIFT;
        hotkeyVirtualKey = 'L';
        unlockPassword = "10203040";
        passwordEnabled = true;
        timerDuration = 60;
        timerEnabled = false;
        whitelistedKeys = "Esc";
        whitelistEnabled = false;
        overlayStyle = 1; // Default to dim overlay
        notificationStyle = 0; // Default to custom notifications
        hideFromTaskbar = true;
        startWithWindows = false;
        usbAlertEnabled = false; // Off by default
        quickLaunchEnabled = false; // Off by default (optimized)
        workBreakTimerEnabled = false; // Off by default
        bossKeyEnabled = false; // Enable by default for better user experience
        bossKeyHotkey = "Ctrl+Alt+F12";
    }
    
    // Comparison operators
    bool operator==(const AppSettings& other) const {
        return keyboardLockEnabled == other.keyboardLockEnabled &&
               mouseLockEnabled == other.mouseLockEnabled &&
               unlockMethod == other.unlockMethod &&
               enableFailsafe == other.enableFailsafe &&
               lockHotkey == other.lockHotkey &&
               hotkeyModifiers == other.hotkeyModifiers &&
               hotkeyVirtualKey == other.hotkeyVirtualKey &&
               unlockPassword == other.unlockPassword &&
               passwordEnabled == other.passwordEnabled &&
               timerDuration == other.timerDuration &&
               timerEnabled == other.timerEnabled &&
               whitelistedKeys == other.whitelistedKeys &&
               whitelistEnabled == other.whitelistEnabled &&
               overlayStyle == other.overlayStyle &&
               notificationStyle == other.notificationStyle &&
               hideFromTaskbar == other.hideFromTaskbar &&
               startWithWindows == other.startWithWindows &&
               usbAlertEnabled == other.usbAlertEnabled &&
               quickLaunchEnabled == other.quickLaunchEnabled &&
               workBreakTimerEnabled == other.workBreakTimerEnabled &&
               bossKeyEnabled == other.bossKeyEnabled &&
               bossKeyHotkey == other.bossKeyHotkey;
    }
    
    bool operator!=(const AppSettings& other) const {
        return !(*this == other);
    }
};

class SettingsCore {
private:
    static const char* REGISTRY_KEY;
    AppSettings defaultSettings;
    
public:
    SettingsCore();
    ~SettingsCore();

    // Core operations
    bool LoadSettings(AppSettings& settings);
    bool SaveSettings(const AppSettings& settings);
    bool ClearPersistentStorage(); // Delete all registry entries (for reset to defaults)
    bool ApplySettings(const AppSettings& settings, HWND mainWindow = NULL);
    bool ApplySettings(const AppSettings& newSettings, const AppSettings& previousSettings, HWND mainWindow = NULL);
    
    // Validation
    bool ValidateSettings(const AppSettings& settings);
    void ResetToDefaults(AppSettings& settings);
    const AppSettings& GetDefaultSettings() const { return defaultSettings; }
    
    // Change detection
    bool HasChanges(const AppSettings& current, const AppSettings& original);
    bool HasHotkeyChanges(const AppSettings& current, const AppSettings& original);
    bool HasLockInputChanges(const AppSettings& current, const AppSettings& original);
    bool HasPrivacyChanges(const AppSettings& current, const AppSettings& original);
    bool HasProductivityChanges(const AppSettings& current, const AppSettings& original);
    bool HasOverlayChanges(const AppSettings& current, const AppSettings& original);
    bool HasNotificationChanges(const AppSettings& current, const AppSettings& original);
    
    // Import/Export
    bool ExportToFile(const AppSettings& settings, const std::string& filepath);
    bool ImportFromFile(AppSettings& settings, const std::string& filepath);
    
    // Systematic layer management
    void UpdateAllLayers(const AppSettings& settings);
    bool IsPersistentDataComplete();
    bool ValidateImportedSettings(const AppSettings& settings);
    
private:
    bool WriteRegistryValue(HKEY hKey, const char* valueName, DWORD value);
    bool ReadRegistryValue(HKEY hKey, const char* valueName, DWORD& value);
    bool WriteRegistryString(HKEY hKey, const char* valueName, const std::string& value);
    bool ReadRegistryString(HKEY hKey, const char* valueName, std::string& value);
    
    // Apply specific setting categories
    bool ApplyHotkeySettings(const AppSettings& settings);
    bool ApplyPrivacySettings(const AppSettings& settings, HWND mainWindow);
    bool ApplyProductivitySettings(const AppSettings& settings, HWND mainWindow);
    bool ApplyOverlaySettings(const AppSettings& settings);
    bool ApplyNotificationSettings(const AppSettings& settings);
};

extern SettingsCore g_settingsCore;
