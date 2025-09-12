// src/utils/hotkey_utils.h
// Hotkey parsing and utility functions

#pragma once
#include <string>
#include <windows.h>

// Utility function to parse hotkey strings (e.g., "Ctrl+Alt+F12")
bool ParseHotkeyString(const std::string& hotkeyStr, UINT& modifiers, UINT& virtualKey);
