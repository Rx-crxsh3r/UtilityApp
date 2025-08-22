// src/settings/timer_manager.cpp
// Timer configuration and management implementation

#include "timer_manager.h"
#include "../resource.h"
#include "../notifications.h"
#include <windows.h>
#include <string>
#include <sstream>
#include <iomanip>

// Global instance
TimerManager g_timerManager;

// Registry constants
const char* TimerManager::REGISTRY_KEY = "SOFTWARE\\UtilityApp\\Timer";

TimerManager::TimerManager() 
    : currentMode(TIMER_DISABLED), timerDuration(300), periodicInterval(1800),
      activeTimerId(0), isTimerActive(false), notificationWindow(NULL), timerStartTime(0) {
    LoadFromRegistry();
}

TimerManager::~TimerManager() {
    StopTimer();
}

bool TimerManager::StartTimer(HWND hwnd) {
    if (currentMode == TIMER_DISABLED || timerDuration <= 0) {
        return false;
    }

    StopTimer(); // Stop any existing timer
    
    notificationWindow = hwnd;
    timerStartTime = GetTickCount();
    
    // Set timer for the specified duration (convert seconds to milliseconds)
    activeTimerId = SetTimer(hwnd, 1001, timerDuration * 1000, TimerProc);
    
    if (activeTimerId != 0) {
        isTimerActive = true;
        
        std::stringstream ss;
        ss << "Timer started for " << this->GetFormattedTime();
        NotifyTimerEvent(ss.str().c_str());
        
        return true;
    }
    
    return false;
}

void TimerManager::StopTimer() {
    if (isTimerActive && activeTimerId != 0) {
        KillTimer(notificationWindow, activeTimerId);
        activeTimerId = 0;
        isTimerActive = false;
        NotifyTimerEvent("Timer stopped");
    }
}

int TimerManager::GetRemainingTime() const {
    if (!isTimerActive) return 0;
    
    DWORD elapsed = (GetTickCount() - timerStartTime) / 1000; // convert to seconds
    int remaining = timerDuration - elapsed;
    return (remaining > 0) ? remaining : 0;
}

std::string TimerManager::GetFormattedTime() const {
    int totalSeconds = isTimerActive ? GetRemainingTime() : timerDuration;
    int minutes = totalSeconds / 60;
    int seconds = totalSeconds % 60;
    
    std::stringstream ss;
    ss << std::setw(2) << std::setfill('0') << minutes 
       << ":" << std::setw(2) << std::setfill('0') << seconds;
    return ss.str();
}

void TimerManager::InitializeTimerControls(HWND hDialog) {
    // Set timer mode radio buttons
    CheckRadioButton(hDialog, IDC_RADIO_TIMER_DISABLED, IDC_RADIO_TIMER_PERIODIC, 
                     IDC_RADIO_TIMER_DISABLED + currentMode);
    
    // Set duration
    SetDlgItemInt(hDialog, IDC_EDIT_TIMER_DURATION, timerDuration, FALSE);
    
    // Set periodic interval
    SetDlgItemInt(hDialog, IDC_EDIT_TIMER_INTERVAL, periodicInterval, FALSE);
    
    // Enable/disable controls based on mode
    BOOL enableDuration = (currentMode != TIMER_DISABLED);
    BOOL enableInterval = (currentMode == TIMER_PERIODIC);
    
    EnableWindow(GetDlgItem(hDialog, IDC_EDIT_TIMER_DURATION), enableDuration);
    EnableWindow(GetDlgItem(hDialog, IDC_EDIT_TIMER_INTERVAL), enableInterval);
    
    // Update display
    UpdateTimerDisplay(hDialog);
}

bool TimerManager::HandleTimerModeChange(HWND hDialog) {
    TimerMode oldMode = currentMode;
    
    for (int i = 0; i < 4; i++) {
        if (IsDlgButtonChecked(hDialog, IDC_RADIO_TIMER_DISABLED + i) == BST_CHECKED) {
            currentMode = (TimerMode)i;
            break;
        }
    }
    
    if (currentMode != oldMode) {
        // Update UI
        InitializeTimerControls(hDialog);
        SaveToRegistry();
        return true;
    }
    
    return false;
}

bool TimerManager::HandleDurationChange(HWND hDialog, int editControlId) {
    BOOL translated;
    int newDuration = GetDlgItemInt(hDialog, editControlId, &translated, FALSE);
    
    if (translated && ValidateDuration(newDuration)) {
        timerDuration = newDuration;
        SaveToRegistry();
        UpdateTimerDisplay(hDialog);
        return true;
    }
    
    // Restore previous value if invalid
    SetDlgItemInt(hDialog, editControlId, timerDuration, FALSE);
    return false;
}

void TimerManager::UpdateTimerDisplay(HWND hDialog) {
    std::string timeStr = this->GetFormattedTime();
    std::string statusText;
    
    if (isTimerActive) {
        statusText = "Timer active - " + timeStr + " remaining";
    } else {
        statusText = "Timer duration: " + timeStr;
    }
    
    SetDlgItemTextA(hDialog, IDC_LABEL_TIMER_STATUS, statusText.c_str());
}

void CALLBACK TimerManager::TimerProc(HWND hwnd, UINT msg, UINT_PTR timerId, DWORD time) {
    g_timerManager.OnTimerExpired();
}

void TimerManager::OnTimerExpired() {
    StopTimer();
    
    switch (currentMode) {
        case TIMER_AUTO_UNLOCK:
            NotifyTimerEvent("Timer expired - Unlocking input");
            // Send unlock message to main window
            if (notificationWindow) {
                PostMessage(notificationWindow, WM_COMMAND, MAKEWPARAM(ID_UNLOCK_INPUT, 0), 0);
            }
            break;
            
        case TIMER_AUTO_LOCK:
            NotifyTimerEvent("Timer expired - Locking input");
            // Send lock message to main window
            if (notificationWindow) {
                PostMessage(notificationWindow, WM_COMMAND, MAKEWPARAM(ID_LOCK_INPUT, 0), 0);
            }
            break;
            
        case TIMER_PERIODIC:
            NotifyTimerEvent("Periodic timer - Toggling input lock");
            // Send toggle message to main window
            if (notificationWindow) {
                PostMessage(notificationWindow, WM_COMMAND, MAKEWPARAM(ID_TOGGLE_INPUT, 0), 0);
            }
            // Restart timer for next cycle
            StartTimer(notificationWindow);
            break;
    }
}

bool TimerManager::LoadFromRegistry() {
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, REGISTRY_KEY, 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
        return false;
    }

    DWORD value, size = sizeof(DWORD), type;
    
    if (RegQueryValueExA(hKey, "Mode", NULL, &type, (BYTE*)&value, &size) == ERROR_SUCCESS) {
        if (value <= TIMER_PERIODIC) currentMode = (TimerMode)value;
    }
    
    if (RegQueryValueExA(hKey, "Duration", NULL, &type, (BYTE*)&value, &size) == ERROR_SUCCESS) {
        if (ValidateDuration(value)) timerDuration = value;
    }
    
    if (RegQueryValueExA(hKey, "Interval", NULL, &type, (BYTE*)&value, &size) == ERROR_SUCCESS) {
        if (ValidateDuration(value)) periodicInterval = value;
    }

    RegCloseKey(hKey);
    return true;
}

bool TimerManager::SaveToRegistry() {
    HKEY hKey;
    if (RegCreateKeyExA(HKEY_CURRENT_USER, REGISTRY_KEY, 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) != ERROR_SUCCESS) {
        return false;
    }

    DWORD value;
    
    value = currentMode;
    RegSetValueExA(hKey, "Mode", 0, REG_DWORD, (const BYTE*)&value, sizeof(DWORD));
    
    value = timerDuration;
    RegSetValueExA(hKey, "Duration", 0, REG_DWORD, (const BYTE*)&value, sizeof(DWORD));
    
    value = periodicInterval;
    RegSetValueExA(hKey, "Interval", 0, REG_DWORD, (const BYTE*)&value, sizeof(DWORD));

    RegCloseKey(hKey);
    return true;
}

bool TimerManager::ValidateDuration(int duration) const {
    return (duration >= 1 && duration <= 86400); // 1 second to 24 hours
}

void TimerManager::NotifyTimerEvent(const char* message) {
    if (message) {
        ShowBalloonTip(GetDesktopWindow(), "Timer Event", message);
    }
}
