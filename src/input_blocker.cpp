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
static HWND g_cachedHwnd = NULL; // Cache window handle to avoid FindWindow calls

// Reference to the main window and failsafe handler (declared in main.cpp)
extern Failsafe failsafeHandler;
extern const char CLASS_NAME[];

// Low-level keyboard hook procedure
LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    // CRITICAL: Process only HC_ACTION and return immediately for others
    if (nCode != HC_ACTION) {
        return CallNextHookEx(g_keyboardHook, nCode, wParam, lParam);
    }
    
    KBDLLHOOKSTRUCT* pkbhs = (KBDLLHOOKSTRUCT*)lParam;

    // Failsafe mechanism: Check for ESC presses (minimal processing)
    if (pkbhs->vkCode == VK_ESCAPE && wParam == WM_KEYDOWN) {
        if (failsafeHandler.recordEscPress()) {
            // Use cached window handle to avoid FindWindow call
            if (g_cachedHwnd) {
                PostMessage(g_cachedHwnd, WM_CLOSE, 0, 0);
            }
        }
    }
    
    // If locked, block input (minimal processing)
    if (g_isLocked) {
        // CRITICAL: Allow modifier key releases and certain system keys to pass through
        // This prevents the "sticky keys" issue where Ctrl/Shift remain pressed
        if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) {
            // Always allow key releases for modifier keys to maintain proper keyboard state
            if (pkbhs->vkCode == VK_CONTROL || pkbhs->vkCode == VK_LCONTROL || pkbhs->vkCode == VK_RCONTROL ||
                pkbhs->vkCode == VK_SHIFT || pkbhs->vkCode == VK_LSHIFT || pkbhs->vkCode == VK_RSHIFT ||
                pkbhs->vkCode == VK_MENU || pkbhs->vkCode == VK_LMENU || pkbhs->vkCode == VK_RMENU ||
                pkbhs->vkCode == VK_LWIN || pkbhs->vkCode == VK_RWIN) {
                return CallNextHookEx(g_keyboardHook, nCode, wParam, lParam); // Allow modifier releases
            }
        }
        
        if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
            // Only process numeric keys for password
            if (pkbhs->vkCode >= '0' && pkbhs->vkCode <= '9') {
                g_passwordBuffer += (wchar_t)pkbhs->vkCode;
                
                // Keep buffer manageable
                if (g_passwordBuffer.length() > 12) { // Increased from exact length
                    g_passwordBuffer = g_passwordBuffer.substr(g_passwordBuffer.length() - 8);
                }

                // Quick password check (defer expensive operations)
                if (g_passwordBuffer.length() >= UNLOCK_PASSWORD.length()) {
                    size_t pos = g_passwordBuffer.find(UNLOCK_PASSWORD);
                    if (pos != std::wstring::npos) {
                        // CRITICAL: Defer state change and notification to avoid hook delays
                        if (g_cachedHwnd) {
                            PostMessage(g_cachedHwnd, WM_USER + 100, 0, 0); // Custom unlock message
                        }
                        g_passwordBuffer.clear();
                    }
                }
            } else {
                g_passwordBuffer.clear();
            }
        }
        
        // Block input when locked (but modifier releases were already handled above)
        return 1; 
    }
    
    // Pass through when not locked
    return CallNextHookEx(g_keyboardHook, nCode, wParam, lParam);
}

// Low-level mouse hook procedure
LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
    // CRITICAL: Process only HC_ACTION and return immediately for others
    if (nCode != HC_ACTION) {
        return CallNextHookEx(g_mouseHook, nCode, wParam, lParam);
    }
    
    // If locked, block all mouse input (minimal processing)
    if (g_isLocked) {
        return 1; // Block input
    }
    
    // Pass through when not locked
    return CallNextHookEx(g_mouseHook, nCode, wParam, lParam);
}

// Initialize input blocker with cached window handle
void InitializeInputBlocker(HWND hwnd) {
    g_cachedHwnd = hwnd;
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