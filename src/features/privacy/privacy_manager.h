// src/features/privacy/privacy_manager.h
// Privacy and window management features

#ifndef PRIVACY_MANAGER_H
#define PRIVACY_MANAGER_H

#include <windows.h>
#include <string>
#include <vector>

// Privacy feature flags
enum PrivacyFeature {
    PRIVACY_HIDE_FROM_TASKBAR = 0x01,
    PRIVACY_HIDE_FROM_ALT_TAB = 0x02,
    PRIVACY_START_WITH_WINDOWS = 0x04,
    PRIVACY_BOSS_KEY = 0x08,
    PRIVACY_MINIMIZE_TO_TRAY = 0x10
};

// Window visibility state for boss key
struct WindowState {
    HWND hwnd;
    bool wasVisible;
    RECT originalRect;
    std::string windowTitle;
};

class PrivacyManager {
private:
    static const char* REGISTRY_KEY;
    static const char* STARTUP_KEY;
    static const char* STARTUP_VALUE_NAME;
    
    // Boss key functionality
    bool bossKeyActive;
    std::vector<WindowState> hiddenWindows;
    UINT bossKeyModifiers;
    UINT bossKeyVirtualKey;
    
    // Window management
    HWND targetWindow;
    DWORD originalExStyle;
    bool isHiddenFromTaskbar;
    bool isHiddenFromAltTab;
    
    // Registry operations
    bool WriteRegistryValue(HKEY hKey, const char* valueName, DWORD value);
    bool ReadRegistryValue(HKEY hKey, const char* valueName, DWORD& value);
    bool WriteStringValue(HKEY hKey, const char* valueName, const std::string& value);
    bool ReadStringValue(HKEY hKey, const char* valueName, std::string& value);
    
    // Window enumeration for boss key
    static BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam);
    
    // Startup management
    bool AddToStartup();
    bool RemoveFromStartup();
    bool IsInStartup();
    
public:
    PrivacyManager();
    ~PrivacyManager();
    
    // Core privacy operations
    bool ApplyPrivacySettings(HWND window, DWORD features);
    bool SetWindowPrivacy(HWND window, bool hideFromTaskbar);
    bool RestoreWindowPrivacy(HWND window);
    
    // Boss key functionality
    bool EnableBossKey(UINT modifiers, UINT virtualKey);
    bool DisableBossKey();
    bool SetBossKeyHotkey(UINT modifiers, UINT virtualKey); // NEW: Allows changing hotkey
    bool ActivateBossKey(); // Hide all windows
    bool DeactivateBossKey(); // Restore all windows
    bool IsBossKeyActive() const { return bossKeyActive; }
    
    // Startup management
    bool SetStartWithWindows(bool enable);
    bool GetStartWithWindows();
    
    // Settings persistence
    bool SaveSettings();
    bool LoadSettings();
    
    // Getters
    bool IsHiddenFromTaskbar() const { return isHiddenFromTaskbar; }
    bool IsHiddenFromAltTab() const { return isHiddenFromAltTab; }
    UINT GetBossKeyModifiers() const { return bossKeyModifiers; }
    UINT GetBossKeyVirtualKey() const { return bossKeyVirtualKey; }
};

// Global instance
extern PrivacyManager g_privacyManager;

#endif // PRIVACY_MANAGER_H
