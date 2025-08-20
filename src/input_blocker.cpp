// src/input_blocker.cpp

#include "input_blocker.h"
#include "failsafe.h"
#include "notifications.h"
#include <string>
#include <vector>

// Global state variables
static HHOOK g_keyboardHook = NULL;
static HHOOK g_mouseHook = NULL;
static bool g_isLocked = false;
static std::wstring g_passwordBuffer = L"";
const std::wstring UNLOCK_PASSWORD = L"10203040";

// Reference to the main window and failsafe handler (declared in main.cpp)
extern Failsafe failsafeHandler;
extern const char CLASS_NAME[];

// Low-level keyboard hook procedure
LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        KBDLLHOOKSTRUCT* pkbhs = (KBDLLHOOKSTRUCT*)lParam;

        // Failsafe mechanism: Check for ESC presses
        if (pkbhs->vkCode == VK_ESCAPE && wParam == WM_KEYDOWN) {
            if (failsafeHandler.recordEscPress()) {
                HWND hwnd = FindWindowA(CLASS_NAME, NULL);
                if(hwnd) {
                    ShowNotification(hwnd, NOTIFY_FAILSAFE_TRIGGERED);
                    PostMessage(hwnd, WM_CLOSE, 0, 0);
                }
            }
        }
        
        // If locked, process input
        if (g_isLocked) {
            if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
                // Check if the key is a number (0-9)
                if (pkbhs->vkCode >= '0' && pkbhs->vkCode <= '9') {
                    g_passwordBuffer += (wchar_t)pkbhs->vkCode;
                    
                    // If buffer exceeds password length, trim it
                    if (g_passwordBuffer.length() > UNLOCK_PASSWORD.length()) {
                        g_passwordBuffer = g_passwordBuffer.substr(g_passwordBuffer.length() - UNLOCK_PASSWORD.length());
                    }

                    // Check for password match
                    if (g_passwordBuffer.find(UNLOCK_PASSWORD) != std::wstring::npos) {
                        g_isLocked = false; // Unlock
                        g_passwordBuffer.clear();
                        HWND hwnd = FindWindowA(CLASS_NAME, NULL);
                        if(hwnd) ShowNotification(hwnd, NOTIFY_INPUT_UNLOCKED);
                    }
                } else {
                    // Reset buffer if a non-numeric key is pressed
                    g_passwordBuffer.clear();
                }
            }
            
            // Block the input by returning a non-zero value
            return 1; 
        }
    }
    
    // Pass the message on to the next hook in the chain
    return CallNextHookEx(g_keyboardHook, nCode, wParam, lParam);
}

// Low-level mouse hook procedure
LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION && g_isLocked) {
        // Block all mouse input when locked
        return 1;
    }
    
    // Pass the message on to the next hook in the chain
    return CallNextHookEx(g_mouseHook, nCode, wParam, lParam);
}

void ToggleInputLock(HWND hwnd) {
    g_isLocked = !g_isLocked;
    g_passwordBuffer.clear(); // Clear buffer on state change
    
    // Show notification
    if (g_isLocked) {
        ShowNotification(hwnd, NOTIFY_INPUT_LOCKED);
    } else {
        ShowNotification(hwnd, NOTIFY_INPUT_UNLOCKED);
    }
}

bool IsInputLocked() {
    return g_isLocked;
}

void InstallHook() {
    if (g_keyboardHook == NULL) {
        g_keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, GetModuleHandle(NULL), 0);
    }
    if (g_mouseHook == NULL) {
        g_mouseHook = SetWindowsHookEx(WH_MOUSE_LL, LowLevelMouseProc, GetModuleHandle(NULL), 0);
    }
}

void UninstallHook() {
    if (g_keyboardHook != NULL) {
        UnhookWindowsHookEx(g_keyboardHook);
        g_keyboardHook = NULL;
    }
    if (g_mouseHook != NULL) {
        UnhookWindowsHookEx(g_mouseHook);
        g_mouseHook = NULL;
    }
}