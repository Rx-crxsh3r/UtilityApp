// src/settings/timer_manager.h
// Timer configuration and management module

#pragma once
#include <windows.h>
#include <string>

enum TimerMode {
    TIMER_DISABLED = 0,
    TIMER_AUTO_UNLOCK = 1,
    TIMER_AUTO_LOCK = 2,
    TIMER_PERIODIC = 3
};

class TimerManager {
private:
    TimerMode currentMode;
    int timerDuration; // in seconds
    int periodicInterval; // for periodic mode
    UINT_PTR activeTimerId;
    bool isTimerActive;
    HWND notificationWindow;

public:
    TimerManager();
    ~TimerManager();

    // Timer configuration
    void SetMode(TimerMode mode) { currentMode = mode; }
    TimerMode GetMode() const { return currentMode; }
    
    void SetDuration(int seconds) { timerDuration = seconds; }
    int GetDuration() const { return timerDuration; }
    
    void SetPeriodicInterval(int seconds) { periodicInterval = seconds; }
    int GetPeriodicInterval() const { return periodicInterval; }

    // Timer control
    bool StartTimer(HWND hwnd);
    void StopTimer();
    bool IsActive() const { return isTimerActive; }
    
    // Get remaining time
    int GetRemainingTime() const;
    std::string GetFormattedTime() const;

    // UI helpers
    void InitializeTimerControls(HWND hDialog);
    bool HandleTimerModeChange(HWND hDialog);
    bool HandleDurationChange(HWND hDialog, int editControlId);
    void UpdateTimerDisplay(HWND hDialog);

    // Timer callback handling
    static void CALLBACK TimerProc(HWND hwnd, UINT msg, UINT_PTR timerId, DWORD time);
    void OnTimerExpired();

    // Load/Save
    bool LoadFromRegistry();
    bool SaveToRegistry();

private:
    static const char* REGISTRY_KEY;
    DWORD timerStartTime;
    
    bool ValidateDuration(int duration) const;
    void NotifyTimerEvent(const char* message);
};

extern TimerManager g_timerManager;
