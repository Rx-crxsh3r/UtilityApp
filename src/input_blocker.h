// src/input_blocker.h

#pragma once
#include <windows.h>

// Toggles the input lock state (locked/unlocked).
void ToggleInputLock(HWND hwnd);

// Returns true if the input is currently locked.
bool IsInputLocked();

// Installs the low-level keyboard hook to capture input.
void InstallHook();

// Uninstalls the low-level keyboard hook.
void UninstallHook();