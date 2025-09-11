// src/features/lock_input/hotkey_manager.cpp
// Hotkey capture and management implementation

#include "hotkey_manager.h"
#include <cstdio>

// Global instance
HotkeyManager g_hotkeyManager;
HotkeyManager* HotkeyManager::instance = nullptr;

HotkeyManager::HotkeyManager() 
    : isCapturing(false), ctrlPressed(false), shiftPressed(false), 
      altPressed(false), winPressed(false), hKeyboardHook(nullptr),
      hDialog(nullptr), hEditControl(nullptr), hHintLabel(nullptr) {
    instance = this;
}

HotkeyManager::~HotkeyManager() {
    if (hKeyboardHook) {
        UnhookWindowsHookEx(hKeyboardHook);
    }
    instance = nullptr;
}

void HotkeyManager::StartCapture(HWND dialog, HWND editControl, HWND hintLabel, const std::string& currentHotkey) {
    if (isCapturing) return;
    
    hDialog = dialog;
    hEditControl = editControl;
    hHintLabel = hintLabel;
    originalHotkey = currentHotkey;
    currentInput = "";
    
    // Reset modifier states
    ctrlPressed = shiftPressed = altPressed = winPressed = false;
    
    // Update UI
    SetWindowTextA(hEditControl, "");
    SetWindowTextA(hHintLabel, "Press key combination...");
    ShowWindow(hHintLabel, SW_SHOW);
    
    // Install hook
    hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, HotkeyHookProc, 
                                    GetModuleHandle(NULL), 0);
    
    isCapturing = true;
}

void HotkeyManager::EndCapture(bool save) {
    if (!isCapturing) return;
    
    // Remove hook
    if (hKeyboardHook) {
        UnhookWindowsHookEx(hKeyboardHook);
        hKeyboardHook = nullptr;
    }
    
    std::string finalHotkey;
    if (save && !currentInput.empty()) {
        finalHotkey = currentInput;
    } else {
        finalHotkey = originalHotkey;
    }
    
    // Update UI
    SetWindowTextA(hEditControl, finalHotkey.c_str());
    ShowWindow(hHintLabel, SW_HIDE);
    
    // Notify parent dialog to update warnings
    if (save && hDialog) {
        PostMessage(hDialog, WM_USER + 101, 0, 0); // Custom message to update warnings
    }
    
    // Remove focus from textbox to deactivate it
    SetFocus(GetParent(hEditControl));
    
    isCapturing = false;
}

void HotkeyManager::UpdateDisplay() {
    if (!isCapturing || !hEditControl) return;
    
    std::string display = FormatHotkey(ctrlPressed, shiftPressed, altPressed, winPressed, "");
    SetWindowTextA(hEditControl, display.c_str());
}

bool HotkeyManager::ValidateHotkey(const std::string& hotkey) {
    // Basic validation - should have at least one modifier
    return !IsSingleKey(hotkey);
}

bool HotkeyManager::IsSingleKey(const std::string& hotkey) {
    return (hotkey.find('+') == std::string::npos) && hotkey.length() == 1;
}

bool HotkeyManager::IsHotkeyAvailable(UINT modifiers, UINT virtualKey) {
    // Try to register the hotkey temporarily to check if it's available
    // Use a unique atom ID for testing
    static const int TEST_HOTKEY_ID = 9999;
    
    // First unregister any existing test hotkey to be safe
    UnregisterHotKey(NULL, TEST_HOTKEY_ID);
    
    // Try to register the hotkey
    BOOL result = RegisterHotKey(NULL, TEST_HOTKEY_ID, modifiers, virtualKey);
    
    // If successful, immediately unregister it
    if (result) {
        UnregisterHotKey(NULL, TEST_HOTKEY_ID);
        return true;
    }
    
    return false;
}

std::string HotkeyManager::VirtualKeyToString(UINT vkCode) {
    // Optimize for common cases - avoid sprintf_s overhead
    if (vkCode >= 'A' && vkCode <= 'Z') {
        return std::string(1, (char)vkCode);
    } else if (vkCode >= '0' && vkCode <= '9') {
        return std::string(1, (char)vkCode);
    }
    
    // Use static strings for common keys to avoid allocations
    switch (vkCode) {
        case VK_ESCAPE: return "Esc";
        case VK_SPACE: return "Space";
        case VK_RETURN: return "Enter";
        case VK_TAB: return "Tab";
        case VK_BACK: return "Backspace";
        case VK_DELETE: return "Delete";
        case VK_HOME: return "Home";
        case VK_END: return "End";
        case VK_PRIOR: return "PageUp";
        case VK_NEXT: return "PageDown";
        case VK_LEFT: return "Left";
        case VK_RIGHT: return "Right";
        case VK_UP: return "Up";
        case VK_DOWN: return "Down";
        case VK_F1: return "F1";
        case VK_F2: return "F2";
        case VK_F3: return "F3";
        case VK_F4: return "F4";
        case VK_F5: return "F5";
        case VK_F6: return "F6";
        case VK_F7: return "F7";
        case VK_F8: return "F8";
        case VK_F9: return "F9";
        case VK_F10: return "F10";
        case VK_F11: return "F11";
        case VK_F12: return "F12";
        default: {
            // Only use sprintf_s for rare cases
            char keyName[16];
            sprintf_s(keyName, sizeof(keyName), "Key%u", vkCode);
            return std::string(keyName);
        }
    }
}

std::string HotkeyManager::FormatHotkey(bool ctrl, bool shift, bool alt, bool win, const std::string& key) {
    // Pre-calculate required size to avoid reallocations
    size_t size = key.length();
    if (ctrl) size += 5;  // "Ctrl+"
    if (shift) size += 6; // "Shift+"
    if (alt) size += 4;   // "Alt+"
    if (win) size += 4;   // "Win+"
    
    std::string result;
    result.reserve(size);
    
    // Build the string efficiently
    if (ctrl) result += "Ctrl+";
    if (shift) result += "Shift+";
    if (alt) result += "Alt+";
    if (win) result += "Win+";
    result += key;
    
    return result;
}

LRESULT CALLBACK HotkeyManager::HotkeyHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
    // Early return for non-action codes or when not capturing
    if (nCode != HC_ACTION) {
        return CallNextHookEx(instance ? instance->hKeyboardHook : NULL, nCode, wParam, lParam);
    }
    
    if (!instance || !instance->isCapturing) {
        return CallNextHookEx(instance->hKeyboardHook, nCode, wParam, lParam);
    }
    
    KBDLLHOOKSTRUCT* pKbd = (KBDLLHOOKSTRUCT*)lParam;
    bool isKeyDown = (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN);
    bool isKeyUp = (wParam == WM_KEYUP || wParam == WM_SYSKEYUP);
    
    // Handle modifier keys with early returns
    if (pKbd->vkCode == VK_CONTROL || pKbd->vkCode == VK_LCONTROL || pKbd->vkCode == VK_RCONTROL) {
        instance->ctrlPressed = isKeyDown;
    } else if (pKbd->vkCode == VK_SHIFT || pKbd->vkCode == VK_LSHIFT || pKbd->vkCode == VK_RSHIFT) {
        instance->shiftPressed = isKeyDown;
    } else if (pKbd->vkCode == VK_MENU || pKbd->vkCode == VK_LMENU || pKbd->vkCode == VK_RMENU) {
        instance->altPressed = isKeyDown;
    } else if (pKbd->vkCode == VK_LWIN || pKbd->vkCode == VK_RWIN) {
        instance->winPressed = isKeyDown;
    } else if (isKeyDown) {
        // Special handling for certain keys
        if (pKbd->vkCode == VK_RETURN) {
            // Enter key pressed - end capture without saving if no modifiers
            if (!instance->ctrlPressed && !instance->shiftPressed && !instance->altPressed && !instance->winPressed) {
                instance->EndCapture(false); // Don't save, restore original
                return 1; // Block the key
            }
        } else if (pKbd->vkCode == VK_ESCAPE) {
            // Escape key - cancel capture
            instance->EndCapture(false);
            return 1; // Block the key
        }
        
        // Non-modifier key pressed - finalize capture
        std::string keyName = instance->VirtualKeyToString(pKbd->vkCode);
        instance->currentInput = instance->FormatHotkey(instance->ctrlPressed, instance->shiftPressed, 
                                                       instance->altPressed, instance->winPressed, keyName);
        instance->EndCapture(true);
        return 1; // Block this key
    }
    
    // Update display only for modifier key events
    if (isKeyDown || isKeyUp) {
        instance->UpdateDisplay();
    }
    
    return 1; // Block all keys during capture
}
