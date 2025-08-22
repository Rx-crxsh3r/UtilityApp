// src/settings/hotkey_manager.h
// Hotkey capture and management system

#pragma once
#include <windows.h>
#include <string>

class HotkeyManager {
private:
    bool isCapturing;
    std::string currentInput;
    std::string originalHotkey;
    bool ctrlPressed, shiftPressed, altPressed, winPressed;
    HHOOK hKeyboardHook;
    HWND hDialog;
    HWND hEditControl;
    HWND hHintLabel;
    
    static HotkeyManager* instance;
    
public:
    HotkeyManager();
    ~HotkeyManager();
    
    // Main hotkey management
    void StartCapture(HWND dialog, HWND editControl, HWND hintLabel, const std::string& currentHotkey);
    void EndCapture(bool save);
    void UpdateDisplay();
    bool IsCapturing() const { return isCapturing; }
    std::string GetCapturedHotkey() const { return currentInput; }
    
    // Validation
    bool ValidateHotkey(const std::string& hotkey);
    bool IsSingleKey(const std::string& hotkey);
    
    // Hook procedure
    static LRESULT CALLBACK HotkeyHookProc(int nCode, WPARAM wParam, LPARAM lParam);
    
    // Utility functions
    static std::string VirtualKeyToString(UINT vkCode);
    static std::string FormatHotkey(bool ctrl, bool shift, bool alt, bool win, const std::string& key);
};

// Global instance
extern HotkeyManager g_hotkeyManager;
