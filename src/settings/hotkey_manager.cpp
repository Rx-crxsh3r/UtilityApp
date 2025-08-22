// src/settings/hotkey_manager.cpp
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

std::string HotkeyManager::VirtualKeyToString(UINT vkCode) {
    char keyName[32];
    
    if (vkCode >= 'A' && vkCode <= 'Z') {
        sprintf_s(keyName, "%c", (char)vkCode);
    } else if (vkCode >= '0' && vkCode <= '9') {
        sprintf_s(keyName, "%c", (char)vkCode);
    } else {
        switch (vkCode) {
            case VK_ESCAPE: strcpy_s(keyName, "Esc"); break;
            case VK_SPACE: strcpy_s(keyName, "Space"); break;
            case VK_RETURN: strcpy_s(keyName, "Enter"); break;
            case VK_TAB: strcpy_s(keyName, "Tab"); break;
            case VK_F1: case VK_F2: case VK_F3: case VK_F4: case VK_F5: case VK_F6:
            case VK_F7: case VK_F8: case VK_F9: case VK_F10: case VK_F11: case VK_F12:
                sprintf_s(keyName, "F%d", vkCode - VK_F1 + 1);
                break;
            default:
                sprintf_s(keyName, "Key%d", vkCode);
                break;
        }
    }
    
    return std::string(keyName);
}

std::string HotkeyManager::FormatHotkey(bool ctrl, bool shift, bool alt, bool win, const std::string& key) {
    std::string result = "";
    if (ctrl) result += "Ctrl+";
    if (shift) result += "Shift+";
    if (alt) result += "Alt+";
    if (win) result += "Win+";
    result += key;
    return result;
}

LRESULT CALLBACK HotkeyManager::HotkeyHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode != HC_ACTION || !instance || !instance->isCapturing) {
        return CallNextHookEx(instance ? instance->hKeyboardHook : NULL, nCode, wParam, lParam);
    }
    
    KBDLLHOOKSTRUCT* pKbd = (KBDLLHOOKSTRUCT*)lParam;
    bool isKeyDown = (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN);
    bool isKeyUp = (wParam == WM_KEYUP || wParam == WM_SYSKEYUP);
    
    // Handle modifier keys
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
        
        // Non-modifier key pressed - finalize
        std::string keyName = VirtualKeyToString(pKbd->vkCode);
        instance->currentInput = FormatHotkey(instance->ctrlPressed, instance->shiftPressed, 
                                             instance->altPressed, instance->winPressed, keyName);
        instance->EndCapture(true);
        return 1; // Block this key
    }
    
    // Update display for modifiers
    if (isKeyDown || isKeyUp) {
        instance->UpdateDisplay();
    }
    
    return 1; // Block all keys during capture
}
