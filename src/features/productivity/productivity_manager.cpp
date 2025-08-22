// src/features/productivity/productivity_manager.cpp
// Productivity enhancement features implementation

#include "productivity_manager.h"
#include "../../notifications.h"
#include "../../custom_notifications.h"
#include "../../audio_manager.h"
#include <dbt.h>
#include <setupapi.h>
#include <cfgmgr32.h>
#include <shellapi.h>
#include <tlhelp32.h>
#include <algorithm>
#include <cstring>

// Registry constants
const char* ProductivityManager::REGISTRY_KEY = "SOFTWARE\\UtilityApp\\Productivity";

// Helper function to find first set bit (replacement for ffs)
static int FindFirstSetBit(DWORD mask) {
    if (mask == 0) return 0;
    int position = 1;
    while ((mask & 1) == 0) {
        mask >>= 1;
        position++;
    }
    return position;
}

ProductivityManager::ProductivityManager()
    : usbAlertEnabled(false), hDeviceNotify(NULL), quickLaunchEnabled(false),
      currentTimerMode(TIMER_DISABLED), workDuration(25), shortBreakDuration(5),
      longBreakDuration(15), pomodoroCount(0), timerStartTime(0), timerId(0),
      timerEnabled(false), fiveMinuteWarningShown(false), notificationWindow(NULL), dndEnabled(false),
      dndDuration(0), dndStartTime(0), mainWindow(NULL) {
    
    LoadSettings();
    
    // Initialize default quick launch apps
    QuickLaunchApp notepad = {"Notepad", "notepad.exe", "", 'N', MOD_CONTROL | MOD_ALT, true};
    QuickLaunchApp calc = {"Calculator", "calc.exe", "", 'C', MOD_CONTROL | MOD_ALT, true};
    QuickLaunchApp explorer = {"File Explorer", "explorer.exe", "", 'E', MOD_CONTROL | MOD_ALT, true};
    
    quickLaunchApps.push_back(notepad);
    quickLaunchApps.push_back(calc);
    quickLaunchApps.push_back(explorer);
}

ProductivityManager::~ProductivityManager() {
    DisableUSBAlert();
    DisableQuickLaunch();
    DisableWorkBreakTimer();
    SaveSettings();
}

bool ProductivityManager::EnableUSBAlert(HWND window) {
    if (usbAlertEnabled) return true;
    
    if (!RegisterForUSBNotifications(window)) {
        return false;
    }
    
    usbAlertEnabled = true;
    return true;
}

bool ProductivityManager::DisableUSBAlert() {
    if (!usbAlertEnabled) return true;
    
    UnregisterUSBNotifications();
    usbAlertEnabled = false;
    detectedDevices.clear();
    return true;
}

bool ProductivityManager::RegisterForUSBNotifications(HWND hwnd) {
    DEV_BROADCAST_DEVICEINTERFACE dbi;
    ZeroMemory(&dbi, sizeof(dbi));
    dbi.dbcc_size = sizeof(dbi);
    dbi.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
    
    hDeviceNotify = RegisterDeviceNotification(hwnd, &dbi, 
                                              DEVICE_NOTIFY_WINDOW_HANDLE | 
                                              DEVICE_NOTIFY_ALL_INTERFACE_CLASSES);
    
    return hDeviceNotify != NULL;
}

void ProductivityManager::UnregisterUSBNotifications() {
    if (hDeviceNotify) {
        UnregisterDeviceNotification(hDeviceNotify);
        hDeviceNotify = NULL;
    }
}

bool ProductivityManager::HandleDeviceChange(WPARAM wParam, LPARAM lParam) {
    if (!usbAlertEnabled) return false;
    
    switch (wParam) {
        case DBT_DEVICEARRIVAL: {
            PDEV_BROADCAST_HDR pHdr = (PDEV_BROADCAST_HDR)lParam;
            if (pHdr->dbch_devicetype == DBT_DEVTYP_VOLUME) {
                PDEV_BROADCAST_VOLUME pVol = (PDEV_BROADCAST_VOLUME)lParam;
                
                // Get drive letter
                char driveLetter = 'A' + (char)(FindFirstSetBit(pVol->dbcv_unitmask) - 1);
                
                USBDevice device;
                device.driveLetter = std::string(1, driveLetter) + ":";
                device.isRemovable = (pVol->dbcv_flags & DBTF_MEDIA) != 0;
                device.insertTime = GetTickCount();
                device.friendlyName = "USB Device (" + device.driveLetter + ")";
                device.deviceId = device.driveLetter;
                
                detectedDevices.push_back(device);
                
                // Show notification with sound
                std::string message = "USB device connected: " + device.driveLetter;
                
                // Force use custom notification system for USB alerts
                if (g_customNotifications) {
                    g_customNotifications->ShowNotification("USB Alert", message);
                }
                
                // Play notification sound
                PlayNotificationSound(SOUND_USB_DEVICE);
                
                return true;
            }
            break;
        }
        case DBT_DEVICEREMOVECOMPLETE: {
            PDEV_BROADCAST_HDR pHdr = (PDEV_BROADCAST_HDR)lParam;
            if (pHdr->dbch_devicetype == DBT_DEVTYP_VOLUME) {
                PDEV_BROADCAST_VOLUME pVol = (PDEV_BROADCAST_VOLUME)lParam;
                
                char driveLetter = 'A' + (char)(FindFirstSetBit(pVol->dbcv_unitmask) - 1);
                std::string driveStr = std::string(1, driveLetter) + ":";
                
                // Remove from detected devices
                detectedDevices.erase(
                    std::remove_if(detectedDevices.begin(), detectedDevices.end(),
                        [&driveStr](const USBDevice& dev) { return dev.driveLetter == driveStr; }),
                    detectedDevices.end());
                
                // Show notification with sound
                std::string message = "USB device removed: " + driveStr;
                
                // Force use custom notification system for USB alerts
                if (g_customNotifications) {
                    g_customNotifications->ShowNotification("USB Alert", message);
                }
                
                // Play notification sound
                PlayNotificationSound(SOUND_USB_DEVICE);
                
                return true;
            }
            break;
        }
    }
    
    return false;
}

bool ProductivityManager::EnableQuickLaunch() {
    if (quickLaunchEnabled) return true;
    
    if (!RegisterQuickLaunchHotkeys()) {
        return false;
    }
    
    quickLaunchEnabled = true;
    return true;
}

bool ProductivityManager::DisableQuickLaunch() {
    if (!quickLaunchEnabled) return true;
    
    UnregisterQuickLaunchHotkeys();
    quickLaunchEnabled = false;
    return true;
}

bool ProductivityManager::RegisterQuickLaunchHotkeys() {
    if (!mainWindow) return false;
    
    // Register hotkeys for quick launch apps
    for (size_t i = 0; i < quickLaunchApps.size(); i++) {
        if (quickLaunchApps[i].enabled) {
            RegisterHotKey(mainWindow, 5000 + i, quickLaunchApps[i].modifiers, quickLaunchApps[i].hotkey);
        }
    }
    return true;
}

void ProductivityManager::UnregisterQuickLaunchHotkeys() {
    if (!mainWindow) return;
    
    for (size_t i = 0; i < quickLaunchApps.size(); i++) {
        UnregisterHotKey(mainWindow, 5000 + i);
    }
}

bool ProductivityManager::AddQuickLaunchApp(const QuickLaunchApp& app) {
    // Check for duplicate names
    for (const auto& existing : quickLaunchApps) {
        if (existing.name == app.name) {
            return false; // Duplicate name
        }
    }
    
    quickLaunchApps.push_back(app);
    
    // Re-register hotkeys if quick launch is enabled
    if (quickLaunchEnabled) {
        UnregisterQuickLaunchHotkeys();
        RegisterQuickLaunchHotkeys();
    }
    
    return true;
}

bool ProductivityManager::RemoveQuickLaunchApp(const std::string& name) {
    auto it = std::find_if(quickLaunchApps.begin(), quickLaunchApps.end(),
        [&name](const QuickLaunchApp& app) { return app.name == name; });
    
    if (it != quickLaunchApps.end()) {
        quickLaunchApps.erase(it);
        
        // Re-register hotkeys
        if (quickLaunchEnabled) {
            UnregisterQuickLaunchHotkeys();
            RegisterQuickLaunchHotkeys();
        }
        
        return true;
    }
    
    return false;
}

bool ProductivityManager::ExecuteQuickLaunchApp(UINT hotkey) {
    for (const auto& app : quickLaunchApps) {
        if (app.enabled && app.hotkey == hotkey) {
            // Launch the application
            SHELLEXECUTEINFOA sei = { 0 };
            sei.cbSize = sizeof(sei);
            sei.fMask = SEE_MASK_NOCLOSEPROCESS;
            sei.lpVerb = "open";
            sei.lpFile = app.path.c_str();
            sei.lpParameters = app.arguments.empty() ? NULL : app.arguments.c_str();
            sei.nShow = SW_SHOWNORMAL;
            
            return ShellExecuteExA(&sei) != FALSE;
        }
    }
    
    return false;
}

bool ProductivityManager::EnableWorkBreakTimer(HWND notifyWindow) {
    if (timerEnabled) return true;
    
    notificationWindow = notifyWindow;
    timerEnabled = true;
    return true;
}

bool ProductivityManager::DisableWorkBreakTimer() {
    if (!timerEnabled) return true;
    
    StopTimer();
    timerEnabled = false;
    notificationWindow = NULL;
    return true;
}

bool ProductivityManager::StartTimer(TimerMode mode) {
    if (!timerEnabled) return false;
    
    StopTimer(); // Stop any existing timer
    
    currentTimerMode = mode;
    timerStartTime = GetTickCount();
    fiveMinuteWarningShown = false; // Reset warning flag for new timer
    
    DWORD duration;
    switch (mode) {
        case TIMER_WORK:
            duration = workDuration * 60 * 1000; // Convert to milliseconds
            break;
        case TIMER_BREAK:
            duration = shortBreakDuration * 60 * 1000;
            break;
        case TIMER_LONG_BREAK:
            duration = longBreakDuration * 60 * 1000;
            break;
        default:
            return false;
    }
    
    timerId = SetTimer(notificationWindow, 2001, duration, TimerProc);
    
    // Set up warning check timer (every 30 seconds)
    SetTimer(notificationWindow, 2002, 30000, TimerProc);
    
    if (timerId != 0) {
        std::string message;
        switch (mode) {
            case TIMER_WORK:
                message = "Work timer started (" + std::to_string(workDuration) + " minutes)";
                break;
            case TIMER_BREAK:
                message = "Break timer started (" + std::to_string(shortBreakDuration) + " minutes)";
                break;
            case TIMER_LONG_BREAK:
                message = "Long break timer started (" + std::to_string(longBreakDuration) + " minutes)";
                break;
        }
        
        if (notificationWindow) {
            ShowNotification(notificationWindow, NOTIFY_INPUT_UNLOCKED, message.c_str());
        }
        
        return true;
    }
    
    return false;
}

bool ProductivityManager::StartWorkSession() {
    if (!timerEnabled) return false;
    
    // Reset Pomodoro count when manually starting
    pomodoroCount = 0;
    return StartTimer(TIMER_WORK);
}

bool ProductivityManager::StopTimer() {
    if (timerId != 0) {
        KillTimer(notificationWindow, timerId);
        KillTimer(notificationWindow, 2002); // Stop warning timer too
        timerId = 0;
        currentTimerMode = TIMER_DISABLED;
        fiveMinuteWarningShown = false; // Reset warning flag
        return true;
    }
    return false;
}

bool ProductivityManager::IsTimerRunning() const {
    return timerId != 0;
}

DWORD ProductivityManager::GetRemainingTime() const {
    if (timerId == 0) return 0;
    
    DWORD elapsed = GetTickCount() - timerStartTime;
    DWORD total;
    
    switch (currentTimerMode) {
        case TIMER_WORK:
            total = workDuration * 60 * 1000;
            break;
        case TIMER_BREAK:
            total = shortBreakDuration * 60 * 1000;
            break;
        case TIMER_LONG_BREAK:
            total = longBreakDuration * 60 * 1000;
            break;
        default:
            return 0;
    }
    
    if (elapsed >= total) return 0;
    return (total - elapsed) / 1000; // Return in seconds
}

void CALLBACK ProductivityManager::TimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime) {
    if (idEvent == 2001) {
        g_productivityManager.HandleTimerExpired();
    } else if (idEvent == 2002) {
        // Check for 5-minute warning every 30 seconds
        g_productivityManager.CheckAndShowFiveMinuteWarning();
    }
}

void ProductivityManager::HandleTimerExpired() {
    std::string message;
    TimerMode nextMode = TIMER_DISABLED;
    
    switch (currentTimerMode) {
        case TIMER_WORK:
            pomodoroCount++;
            if (pomodoroCount % 4 == 0) {
                message = "Work session complete! Starting long break (" + std::to_string(longBreakDuration) + " min).";
                nextMode = TIMER_LONG_BREAK;
            } else {
                message = "Work session complete! Starting short break (" + std::to_string(shortBreakDuration) + " min).";
                nextMode = TIMER_BREAK;
            }
            break;
        case TIMER_BREAK:
            message = "Break time over! Starting new work session (" + std::to_string(workDuration) + " min).";
            nextMode = TIMER_WORK;
            break;
        case TIMER_LONG_BREAK:
            message = "Long break over! Starting new work session (" + std::to_string(workDuration) + " min).";
            nextMode = TIMER_WORK;
            break;
    }
    
    StopTimer();
    
    if (notificationWindow) {
        ShowNotification(notificationWindow, NOTIFY_INPUT_UNLOCKED, message.c_str());
    }
    
    // Play work/break timer sound
    PlayNotificationSound(SOUND_WORK_BREAK);
    
    // Auto-start next timer for proper Pomodoro workflow
    if (nextMode != TIMER_DISABLED) {
        StartTimer(nextMode);
    }
}

void ProductivityManager::CheckAndShowFiveMinuteWarning() {
    if (!timerEnabled || currentTimerMode != TIMER_WORK || fiveMinuteWarningShown) {
        return; // Only show warning during work sessions and only once
    }
    
    DWORD remainingSeconds = GetRemainingTime();
    
    // Show warning if 5 minutes (300 seconds) or less remaining
    if (remainingSeconds <= 300 && remainingSeconds > 0) {
        fiveMinuteWarningShown = true;
        
        std::string warningMessage = "Work session ending in " + 
                                   std::to_string(remainingSeconds / 60) + 
                                   " minutes. Prepare for break!";
        
        // Force use custom notification for timer warnings
        if (g_customNotifications) {
            g_customNotifications->ShowNotification("Break Warning", warningMessage);
        }
        
        // Play notification sound
        PlayNotificationSound(SOUND_WORK_BREAK);
    }
}

bool ProductivityManager::EnableDND(DWORD duration) {
    dndEnabled = true;
    dndDuration = duration;
    dndStartTime = GetTickCount();
    
    if (notificationWindow) {
        std::string message = duration > 0 
            ? "Do Not Disturb enabled for " + std::to_string(duration) + " minutes"
            : "Do Not Disturb enabled indefinitely";
        ShowNotification(notificationWindow, NOTIFY_INPUT_UNLOCKED, message.c_str());
    }
    
    return true;
}

bool ProductivityManager::DisableDND() {
    if (!dndEnabled) return true;
    
    dndEnabled = false;
    
    if (notificationWindow) {
        ShowNotification(notificationWindow, NOTIFY_INPUT_UNLOCKED, "Do Not Disturb disabled");
    }
    
    return true;
}

bool ProductivityManager::IsDNDActive() const {
    if (!dndEnabled) return false;
    
    if (dndDuration == 0) return true; // Indefinite
    
    DWORD elapsed = (GetTickCount() - dndStartTime) / (60 * 1000); // minutes
    return elapsed < dndDuration;
}

DWORD ProductivityManager::GetDNDRemainingTime() const {
    if (!dndEnabled || dndDuration == 0) return 0;
    
    DWORD elapsed = (GetTickCount() - dndStartTime) / (60 * 1000); // minutes
    if (elapsed >= dndDuration) return 0;
    
    return (dndDuration - elapsed) * 60; // Return in seconds
}

bool ProductivityManager::SaveSettings() {
    HKEY hKey;
    if (RegCreateKeyExA(HKEY_CURRENT_USER, REGISTRY_KEY, 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) != ERROR_SUCCESS) {
        return false;
    }
    
    bool success = true;
    success &= WriteRegistryValue(hKey, "USBAlertEnabled", usbAlertEnabled ? 1 : 0);
    success &= WriteRegistryValue(hKey, "QuickLaunchEnabled", quickLaunchEnabled ? 1 : 0);
    success &= WriteRegistryValue(hKey, "TimerEnabled", timerEnabled ? 1 : 0);
    success &= WriteRegistryValue(hKey, "WorkDuration", workDuration);
    success &= WriteRegistryValue(hKey, "ShortBreakDuration", shortBreakDuration);
    success &= WriteRegistryValue(hKey, "LongBreakDuration", longBreakDuration);
    
    RegCloseKey(hKey);
    return success;
}

bool ProductivityManager::LoadSettings() {
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, REGISTRY_KEY, 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
        return false; // Use defaults
    }
    
    DWORD value;
    if (ReadRegistryValue(hKey, "USBAlertEnabled", value)) {
        usbAlertEnabled = (value != 0);
    }
    if (ReadRegistryValue(hKey, "QuickLaunchEnabled", value)) {
        quickLaunchEnabled = (value != 0);
    }
    if (ReadRegistryValue(hKey, "TimerEnabled", value)) {
        timerEnabled = (value != 0);
    }
    if (ReadRegistryValue(hKey, "WorkDuration", value)) {
        workDuration = value;
    }
    if (ReadRegistryValue(hKey, "ShortBreakDuration", value)) {
        shortBreakDuration = value;
    }
    if (ReadRegistryValue(hKey, "LongBreakDuration", value)) {
        longBreakDuration = value;
    }
    
    RegCloseKey(hKey);
    return true;
}

bool ProductivityManager::WriteRegistryValue(HKEY hKey, const char* valueName, DWORD value) {
    return RegSetValueExA(hKey, valueName, 0, REG_DWORD, (const BYTE*)&value, sizeof(DWORD)) == ERROR_SUCCESS;
}

bool ProductivityManager::ReadRegistryValue(HKEY hKey, const char* valueName, DWORD& value) {
    DWORD size = sizeof(DWORD), type;
    return RegQueryValueExA(hKey, valueName, NULL, &type, (BYTE*)&value, &size) == ERROR_SUCCESS && type == REG_DWORD;
}

bool ProductivityManager::WriteStringValue(HKEY hKey, const char* valueName, const std::string& value) {
    return RegSetValueExA(hKey, valueName, 0, REG_SZ, 
                         (const BYTE*)value.c_str(), value.length() + 1) == ERROR_SUCCESS;
}

bool ProductivityManager::ReadStringValue(HKEY hKey, const char* valueName, std::string& value) {
    char buffer[512];
    DWORD bufferSize = sizeof(buffer);
    DWORD type;
    
    if (RegQueryValueExA(hKey, valueName, NULL, &type, (BYTE*)buffer, &bufferSize) == ERROR_SUCCESS && type == REG_SZ) {
        value = buffer;
        return true;
    }
    
    return false;
}
