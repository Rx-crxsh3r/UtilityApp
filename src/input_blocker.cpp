// src/input_blocker.cpp

#include "input_blocker.h"
#include "notifications.h"
#include "failsafe.h"
#include "settings.h"
#include "overlay.h"
#include "features/lock_input/timer_manager.h"
#include "features/lock_input/password_manager.h"
#include <string>
#include <vector>

// Global state variables
static HHOOK g_keyboardHook = NULL;
static HHOOK g_mouseHook = NULL;
static bool g_isLocked = false;
std::wstring g_passwordBuffer = L""; // Remove static to match extern declaration
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
    
    // If locked, check individual keyboard/mouse settings before blocking
    if (g_isLocked) {
        // Only block keyboard input if keyboard lock is enabled
        if (!g_appSettings.keyboardLockEnabled) {
            return CallNextHookEx(g_keyboardHook, nCode, wParam, lParam);
        }
        
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
            // First check whitelist if enabled (addon feature)
            if (g_appSettings.whitelistEnabled) {
                // Check if this key is whitelisted
                // For now, allow ESC and function keys through for emergency access
                if (pkbhs->vkCode == VK_ESCAPE || 
                    (pkbhs->vkCode >= VK_F1 && pkbhs->vkCode <= VK_F12)) {
                    return CallNextHookEx(g_keyboardHook, nCode, wParam, lParam);
                }
            }
            
            // Handle unlock methods based on current settings (Password or Timer only)
            switch (g_appSettings.unlockMethod) {
                case 0: { // Password unlock method
                    // Handle alphanumeric input for flexible passwords
                    if ((pkbhs->vkCode >= '0' && pkbhs->vkCode <= '9') ||
                        (pkbhs->vkCode >= 'A' && pkbhs->vkCode <= 'Z')) {
                        
                        g_passwordBuffer += (wchar_t)pkbhs->vkCode;
                        
                        // Optimized buffer management - keep reasonable size
                        const size_t MAX_BUFFER_SIZE = 20;
                        if (g_passwordBuffer.length() > MAX_BUFFER_SIZE) {
                            g_passwordBuffer = g_passwordBuffer.substr(g_passwordBuffer.length() - 16);
                        }

                        // Check custom password first if set
                        extern PasswordManager g_passwordManager;
                        if (g_passwordManager.HasPassword() && g_passwordBuffer.length() >= 3) {
                            // Check every character after minimum length to catch passwords early
                            if (g_cachedHwnd) {
                                PostMessage(g_cachedHwnd, WM_USER + 101, (WPARAM)g_passwordBuffer.length(), 0);
                            }
                        }
                        
                        // Only check default password if no custom password is set
                        if (!g_passwordManager.HasPassword() && g_passwordBuffer.length() >= UNLOCK_PASSWORD.length()) {
                            size_t pos = g_passwordBuffer.find(UNLOCK_PASSWORD);
                            if (pos != std::wstring::npos) {
                                // CRITICAL: Defer unlock to avoid hook delays
                                if (g_cachedHwnd) {
                                    PostMessage(g_cachedHwnd, WM_USER + 100, 0, 0);
                                }
                                g_passwordBuffer.clear();
                                return 1; // Block this keypress but allow unlock
                            }
                        }
                    } else {
                        // Non-alphanumeric key - clear buffer quickly and efficiently
                        if (!g_passwordBuffer.empty()) {
                            g_passwordBuffer.clear();
                            g_passwordBuffer.reserve(20); // Pre-allocate for next attempt
                        }
                    }
                    break;
                }
                
                case 1: // Timer unlock method - no keyboard processing needed
                    // Timer unlocking is handled by TimerManager, just clear password buffer
                    if (!g_passwordBuffer.empty()) {
                        g_passwordBuffer.clear();
                        g_passwordBuffer.reserve(20);
                    }
                    break;
                    
                default:
                    if (!g_passwordBuffer.empty()) {
                        g_passwordBuffer.clear();
                        g_passwordBuffer.reserve(20);
                    }
                    break;
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
    
    // If locked, check mouse lock settings before blocking
    if (g_isLocked) {
        // Only block mouse input if mouse lock is enabled
        if (!g_appSettings.mouseLockEnabled) {
            return CallNextHookEx(g_mouseHook, nCode, wParam, lParam);
        }
        
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
    
    // OPTIMIZATION: Clear password buffer immediately on state change for better responsiveness
    g_passwordBuffer.clear();
    g_passwordBuffer.reserve(20); // Pre-allocate to avoid repeated allocations
    
    // Show/hide overlay based on lock state and settings
    if (g_isLocked) {
        g_screenOverlay.ShowOverlay((OverlayStyle)g_appSettings.overlayStyle);
        ShowNotification(hwnd, NOTIFY_INPUT_LOCKED);
        
        // Start timer if timer unlock method is selected
        if (g_appSettings.unlockMethod == 1 && g_appSettings.timerEnabled) {
            extern TimerManager g_timerManager;
            g_timerManager.StartTimer(hwnd);
        }
    } else {
        g_screenOverlay.HideOverlay();
        ShowNotification(hwnd, NOTIFY_INPUT_UNLOCKED);
        
        // Stop timer when unlocked
        extern TimerManager g_timerManager;
        g_timerManager.StopTimer();
    }
}

bool IsInputLocked() {
    return g_isLocked;
}

void InstallHook() {
    // ALWAYS install keyboard hook for failsafe mechanism
    // Failsafe (ESC x3) must be available regardless of lock state
    if (g_keyboardHook == NULL) {
        g_keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, GetModuleHandle(NULL), 0);
    }
    // Only install mouse hook if mouse lock is enabled
    if (g_appSettings.mouseLockEnabled && g_mouseHook == NULL) {
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

// Refresh hooks when settings change
void RefreshHooks() {
    // Only uninstall mouse hook if it's no longer needed
    if (!g_appSettings.mouseLockEnabled && g_mouseHook != NULL) {
        UnhookWindowsHookEx(g_mouseHook);
        g_mouseHook = NULL;
    }
    
    // Always ensure keyboard hook is installed for failsafe
    // Install any hooks that are now enabled
    InstallHook();
}