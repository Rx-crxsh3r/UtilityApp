// src/features/privacy/privacy_manager.cpp
// Privacy and window management implementation

#include "privacy_manager.h"
#include "../../notifications.h"
#include <shlobj.h>
#include <iostream>

// Registry constants
const char* PrivacyManager::REGISTRY_KEY = "SOFTWARE\\UtilityApp\\Privacy";
const char* PrivacyManager::STARTUP_KEY = "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run";
const char* PrivacyManager::STARTUP_VALUE_NAME = "UtilityApp";

PrivacyManager::PrivacyManager()
    : bossKeyActive(false), targetWindow(NULL), mainWindow(NULL), originalExStyle(0),
      isHiddenFromTaskbar(false), isHiddenFromAltTab(false),
      bossKeyModifiers(MOD_CONTROL | MOD_SHIFT), bossKeyVirtualKey('B') {
    LoadSettings();
}

PrivacyManager::~PrivacyManager() {
    if (bossKeyActive) {
        DeactivateBossKey();
    }
    SaveSettings();
}

bool PrivacyManager::ApplyPrivacySettings(HWND window, DWORD features) {
    if (!window) return false;
    
    targetWindow = window;
    bool success = true;
    
    // Apply taskbar visibility
    if (features & PRIVACY_HIDE_FROM_TASKBAR) {
        success &= SetWindowPrivacy(window, true);
    } else {
        success &= SetWindowPrivacy(window, false);
    }
    
    // Apply startup setting
    if (features & PRIVACY_START_WITH_WINDOWS) {
        success &= SetStartWithWindows(true);
    }
    
    // Apply boss key
    if (features & PRIVACY_BOSS_KEY) {
        success &= SetBossKeyHotkey(bossKeyModifiers, bossKeyVirtualKey);
    }
    
    return success;
}

bool PrivacyManager::SetWindowPrivacy(HWND window, bool hideFromTaskbar) {
    if (!window) return false;
    
    // Store original style if not already stored
    if (targetWindow != window) {
        originalExStyle = GetWindowLong(window, GWL_EXSTYLE);
        targetWindow = window;
    }
    
    LONG exStyle = GetWindowLong(window, GWL_EXSTYLE);
    
    // Clear any existing WS_EX_TOOLWINDOW first for clean state
    exStyle &= ~WS_EX_TOOLWINDOW;
    
    // Apply WS_EX_TOOLWINDOW only if hiding from taskbar is enabled
    if (hideFromTaskbar) {
        exStyle |= WS_EX_TOOLWINDOW;
    }
    
    // Update internal state
    isHiddenFromTaskbar = hideFromTaskbar;
    
    // Apply the changes
    SetWindowLong(window, GWL_EXSTYLE, exStyle);
    
    // Force window refresh without the hide/show flicker that can cause issues
    SetWindowPos(window, NULL, 0, 0, 0, 0, 
                SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
    
    return true;
}

bool PrivacyManager::RestoreWindowPrivacy(HWND window) {
    if (!window || targetWindow != window) return false;
    
    SetWindowLong(window, GWL_EXSTYLE, originalExStyle);
    isHiddenFromTaskbar = false;
    isHiddenFromAltTab = false;
    
    // Force window style update
    ShowWindow(window, SW_HIDE);
    ShowWindow(window, SW_SHOW);
    
    return true;
}

bool PrivacyManager::EnableBossKey(UINT modifiers, UINT virtualKey) {
    bossKeyModifiers = modifiers;
    bossKeyVirtualKey = virtualKey;
    
    // Register global hotkey for boss key with main window
    if (!RegisterHotKey(mainWindow, 9001, modifiers, virtualKey)) {
        return false;
    }
    
    return true;
}

bool PrivacyManager::DisableBossKey() {
    if (bossKeyActive) {
        DeactivateBossKey();
    }
    
    UnregisterHotKey(mainWindow, 9001);
    return true;
}

bool PrivacyManager::SetBossKeyHotkey(UINT modifiers, UINT virtualKey) {
    // Unregister old hotkey
    UnregisterHotKey(mainWindow, 9001);
    
    // Update the hotkey values
    bossKeyModifiers = modifiers;
    bossKeyVirtualKey = virtualKey;
    
    // Re-register with new hotkey
    if (!RegisterHotKey(mainWindow, 9001, modifiers, virtualKey)) {
        return false;
    }
    
    // Save the new hotkey to registry
    SaveSettings();
    return true;
}

bool PrivacyManager::ActivateBossKey() {
    if (bossKeyActive) return true;
    
    // Pre-allocate vector for better performance
    hiddenWindows.clear();
    hiddenWindows.reserve(50); // Reserve space for typical window count
    
    // Enumerate all visible windows and hide them efficiently
    EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(this));
    
    bossKeyActive = true;
    
    // Show notification
    ShowNotification(mainWindow, NOTIFY_BOSS_KEY_ACTIVATED);
    
    return true;
}

bool PrivacyManager::DeactivateBossKey() {
    if (!bossKeyActive) return true;
    
    // Restore all hidden windows
    for (const auto& windowState : hiddenWindows) {
        if (IsWindow(windowState.hwnd) && windowState.wasVisible) {
            ShowWindow(windowState.hwnd, SW_SHOW);
            SetWindowPos(windowState.hwnd, HWND_TOP, 
                        windowState.originalRect.left, windowState.originalRect.top,
                        windowState.originalRect.right - windowState.originalRect.left,
                        windowState.originalRect.bottom - windowState.originalRect.top,
                        SWP_SHOWWINDOW);
        }
    }
    
    hiddenWindows.clear();
    bossKeyActive = false;
    
    // Show notification
    ShowNotification(mainWindow, NOTIFY_BOSS_KEY_DEACTIVATED);
    
    return true;
}

BOOL CALLBACK PrivacyManager::EnumWindowsProc(HWND hwnd, LPARAM lParam) {
    PrivacyManager* manager = reinterpret_cast<PrivacyManager*>(lParam);
    
    // Quick checks first for performance - fail fast
    if (!IsWindowVisible(hwnd)) return TRUE;
    if (GetParent(hwnd) != NULL) return TRUE; // Skip child windows
    
    // Quick style check to skip windows without title bar (most efficient filter)
    LONG style = GetWindowLong(hwnd, GWL_STYLE);
    if (!(style & WS_CAPTION)) return TRUE;
    
    // Quick class name check to skip system windows (using faster, shorter buffer)
    char className[32]; // Reduced from 64 to 32 for faster allocation
    int classNameLen = GetClassNameA(hwnd, className, sizeof(className));
    if (classNameLen > 0) {
        // Use switch on first character for faster filtering
        switch (className[0]) {
            case 'S': // Shell_TrayWnd
                if (strcmp(className, "Shell_TrayWnd") == 0) return TRUE;
                break;
            case 'P': // Progman
                if (strcmp(className, "Progman") == 0) return TRUE;
                break;
            case 'W': // WorkerW
                if (strcmp(className, "WorkerW") == 0) return TRUE;
                break;
            case 'B': // Button
                if (strncmp(className, "Button", 6) == 0) return TRUE;
                break;
        }
    }
    
    // Only store essential info and hide immediately for performance
    WindowState state;
    state.hwnd = hwnd;
    state.wasVisible = true; // We already verified this
    GetWindowRect(hwnd, &state.originalRect);
    state.windowTitle = ""; // Skip title for performance - not needed for restore
    
    // Hide window immediately to reduce UI flicker
    ShowWindow(hwnd, SW_HIDE);
    
    // Store state for later restoration (do this after hiding for responsiveness)
    manager->hiddenWindows.push_back(state);
    
    return TRUE; // Continue enumeration
}

bool PrivacyManager::SetStartWithWindows(bool enable) {
    if (enable) {
        return AddToStartup();
    } else {
        return RemoveFromStartup();
    }
}

bool PrivacyManager::GetStartWithWindows() {
    return IsInStartup();
}

bool PrivacyManager::AddToStartup() {
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, STARTUP_KEY, 0, KEY_WRITE, &hKey) != ERROR_SUCCESS) {
        return false;
    }
    
    // Get current executable path
    char exePath[MAX_PATH];
    GetModuleFileNameA(NULL, exePath, MAX_PATH);
    
    // Add quotes to handle paths with spaces
    std::string quotedPath = "\"" + std::string(exePath) + "\"";
    
    bool success = WriteStringValue(hKey, STARTUP_VALUE_NAME, quotedPath);
    RegCloseKey(hKey);
    
    return success;
}

bool PrivacyManager::RemoveFromStartup() {
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, STARTUP_KEY, 0, KEY_WRITE, &hKey) != ERROR_SUCCESS) {
        return false;
    }
    
    LONG result = RegDeleteValueA(hKey, STARTUP_VALUE_NAME);
    RegCloseKey(hKey);
    
    return result == ERROR_SUCCESS || result == ERROR_FILE_NOT_FOUND;
}

bool PrivacyManager::IsInStartup() {
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, STARTUP_KEY, 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
        return false;
    }
    
    std::string value;
    bool exists = ReadStringValue(hKey, STARTUP_VALUE_NAME, value);
    RegCloseKey(hKey);
    
    return exists && !value.empty();
}

bool PrivacyManager::SaveSettings() {
    HKEY hKey;
    if (RegCreateKeyExA(HKEY_CURRENT_USER, REGISTRY_KEY, 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) != ERROR_SUCCESS) {
        return false;
    }
    
    bool success = true;
    success &= WriteRegistryValue(hKey, "BossKeyModifiers", bossKeyModifiers);
    success &= WriteRegistryValue(hKey, "BossKeyVirtualKey", bossKeyVirtualKey);
    success &= WriteRegistryValue(hKey, "HideFromTaskbar", isHiddenFromTaskbar ? 1 : 0);
    success &= WriteRegistryValue(hKey, "HideFromAltTab", isHiddenFromAltTab ? 1 : 0);
    
    RegCloseKey(hKey);
    return success;
}

bool PrivacyManager::LoadSettings() {
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, REGISTRY_KEY, 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
        return false; // Use defaults
    }
    
    DWORD value;
    if (ReadRegistryValue(hKey, "BossKeyModifiers", value)) {
        bossKeyModifiers = value;
    }
    if (ReadRegistryValue(hKey, "BossKeyVirtualKey", value)) {
        bossKeyVirtualKey = value;
    }
    if (ReadRegistryValue(hKey, "HideFromTaskbar", value)) {
        isHiddenFromTaskbar = (value != 0);
    }
    if (ReadRegistryValue(hKey, "HideFromAltTab", value)) {
        isHiddenFromAltTab = (value != 0);
    }
    
    RegCloseKey(hKey);
    return true;
}

bool PrivacyManager::WriteRegistryValue(HKEY hKey, const char* valueName, DWORD value) {
    return RegSetValueExA(hKey, valueName, 0, REG_DWORD, (const BYTE*)&value, sizeof(DWORD)) == ERROR_SUCCESS;
}

bool PrivacyManager::ReadRegistryValue(HKEY hKey, const char* valueName, DWORD& value) {
    DWORD size = sizeof(DWORD), type;
    return RegQueryValueExA(hKey, valueName, NULL, &type, (BYTE*)&value, &size) == ERROR_SUCCESS && type == REG_DWORD;
}

bool PrivacyManager::WriteStringValue(HKEY hKey, const char* valueName, const std::string& value) {
    return RegSetValueExA(hKey, valueName, 0, REG_SZ, 
                         (const BYTE*)value.c_str(), value.length() + 1) == ERROR_SUCCESS;
}

bool PrivacyManager::ReadStringValue(HKEY hKey, const char* valueName, std::string& value) {
    char buffer[512];
    DWORD bufferSize = sizeof(buffer);
    DWORD type;
    
    if (RegQueryValueExA(hKey, valueName, NULL, &type, (BYTE*)buffer, &bufferSize) == ERROR_SUCCESS && type == REG_SZ) {
        value = buffer;
        return true;
    }
    
    return false;
}
