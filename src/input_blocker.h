// src/input_blocker.h

#pragma once
#include <windows.h>
#include <string>

// Initialize the input blocker with cached window handle for performance
void InitializeInputBlocker(HWND hwnd);

// Toggles the input lock state (locked/unlocked).
void ToggleInputLock(HWND hwnd);

// Returns true if the input is currently locked.
bool IsInputLocked();

// Installs the low-level keyboard hook to capture input.
void InstallHook();

// Uninstalls the low-level keyboard hook.
void UninstallHook();

// Refresh hooks when settings change (reinstalls based on current settings)
void RefreshHooks();

// Access to password buffer for custom validation (extern declaration)
extern std::wstring g_passwordBuffer;