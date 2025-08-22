// src/features/productivity/productivity_manager.h
// Productivity enhancement features

#ifndef PRODUCTIVITY_MANAGER_H
#define PRODUCTIVITY_MANAGER_H

#include <windows.h>
#include <string>
#include <vector>

// USB Device detection structure
struct USBDevice {
    std::string deviceId;
    std::string friendlyName;
    std::string driveLetter;
    bool isRemovable;
    DWORD insertTime;
};

// Quick launch app entry
struct QuickLaunchApp {
    std::string name;
    std::string path;
    std::string arguments;
    UINT hotkey; // Virtual key code
    UINT modifiers; // MOD_CONTROL, etc.
    bool enabled;
};

// Work/Break timer states
enum TimerMode {
    TIMER_WORK = 0,
    TIMER_BREAK = 1,
    TIMER_LONG_BREAK = 2,
    TIMER_DISABLED = 3
};

class ProductivityManager {
private:
    static const char* REGISTRY_KEY;
    
    // USB Alert system
    bool usbAlertEnabled;
    std::vector<USBDevice> detectedDevices;
    HDEVNOTIFY hDeviceNotify;
    
    // Quick Launch system
    std::vector<QuickLaunchApp> quickLaunchApps;
    bool quickLaunchEnabled;
    
    // Work/Break Timer (Pomodoro-style)
    TimerMode currentTimerMode;
    DWORD workDuration;      // minutes
    DWORD shortBreakDuration; // minutes  
    DWORD longBreakDuration;  // minutes
    DWORD pomodoroCount;
    DWORD timerStartTime;
    UINT timerId;
    bool timerEnabled;
    bool fiveMinuteWarningShown; // Track if 5-minute warning was shown
    HWND notificationWindow;
    
    // Do Not Disturb Mode
    bool dndEnabled;
    DWORD dndDuration; // minutes, 0 = indefinite
    DWORD dndStartTime;
    
    // Main window handle for hotkeys and notifications
    HWND mainWindow;
    
    // Registry operations
    bool WriteRegistryValue(HKEY hKey, const char* valueName, DWORD value);
    bool ReadRegistryValue(HKEY hKey, const char* valueName, DWORD& value);
    bool WriteStringValue(HKEY hKey, const char* valueName, const std::string& value);
    bool ReadStringValue(HKEY hKey, const char* valueName, std::string& value);
    
    // USB notification handling
    bool RegisterForUSBNotifications(HWND hwnd);
    void UnregisterUSBNotifications();
    void ProcessUSBDevice(WPARAM wParam, LPARAM lParam);
    
    // Quick launch management
    bool RegisterQuickLaunchHotkeys();
    void UnregisterQuickLaunchHotkeys();
    
    // Timer callbacks
    static void CALLBACK TimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);
    void HandleTimerExpired();
    void CheckAndShowFiveMinuteWarning(); // Check for 5-minute warning
    
public:
    ProductivityManager();
    ~ProductivityManager();
    
    // Initialization
    void SetMainWindow(HWND window) { mainWindow = window; }
    
    // USB Alert functionality
    bool EnableUSBAlert(HWND window);
    bool DisableUSBAlert();
    bool HandleDeviceChange(WPARAM wParam, LPARAM lParam);
    std::vector<USBDevice> GetDetectedDevices() const { return detectedDevices; }
    
    // Quick Launch functionality
    bool EnableQuickLaunch();
    bool DisableQuickLaunch();
    bool AddQuickLaunchApp(const QuickLaunchApp& app);
    bool RemoveQuickLaunchApp(const std::string& name);
    bool ExecuteQuickLaunchApp(UINT hotkey);
    std::vector<QuickLaunchApp> GetQuickLaunchApps() const { return quickLaunchApps; }
    
    // Work/Break Timer functionality
    bool EnableWorkBreakTimer(HWND notifyWindow);
    bool DisableWorkBreakTimer();
    bool StartTimer(TimerMode mode);
    bool StartWorkSession(); // NEW: Convenience method to start a work session
    bool StopTimer();
    bool IsTimerRunning() const;
    TimerMode GetCurrentTimerMode() const { return currentTimerMode; }
    DWORD GetRemainingTime() const; // in seconds
    
    // Do Not Disturb functionality
    bool EnableDND(DWORD duration = 0); // 0 = indefinite
    bool DisableDND();
    bool IsDNDActive() const;
    DWORD GetDNDRemainingTime() const; // in seconds
    
    // Settings persistence
    bool SaveSettings();
    bool LoadSettings();
    
    // Getters for UI
    bool IsUSBAlertEnabled() const { return usbAlertEnabled; }
    bool IsQuickLaunchEnabled() const { return quickLaunchEnabled; }
    bool IsTimerEnabled() const { return timerEnabled; }
    DWORD GetWorkDuration() const { return workDuration; }
    DWORD GetShortBreakDuration() const { return shortBreakDuration; }
    DWORD GetLongBreakDuration() const { return longBreakDuration; }
};

// Global instance
extern ProductivityManager g_productivityManager;

#endif // PRODUCTIVITY_MANAGER_H
