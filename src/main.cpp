// src/main.cpp

#include <windows.h>
#include <algorithm>
#include <string>
#include <cctype>
#include "resource.h"
#include "tray_icon.h"
#include "input_blocker.h"
#include "failsafe.h"
#include "notifications.h"
#include "settings.h"
#include "overlay.h"
#include "custom_notifications.h"
#include "audio_manager.h"
#include "features/productivity/productivity_manager.h"
#include "features/privacy/privacy_manager.h"

// Forward declarations
extern Failsafe failsafeHandler;
extern const char CLASS_NAME[];
extern HWND g_mainWindow;

// Function to register hotkeys based on current settings
void RegisterHotkeyFromSettings(HWND hwnd);

// Utility function to parse hotkey strings (e.g., "Ctrl+Alt+F12")
bool ParseHotkeyString(const std::string& hotkeyStr, UINT& modifiers, UINT& virtualKey);
#include "settings/password_manager.h"

// Global variables
extern const char CLASS_NAME[] = "UtilityAppClass";
Failsafe failsafeHandler;
HWND g_mainWindow = NULL; // Global main window handle for settings updates

// Global manager instances
ProductivityManager g_productivityManager;
PrivacyManager g_privacyManager;

// Function declarations
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void RegisterHotkeyFromSettings(HWND hwnd);

// Window Procedure
LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE: {
            // Initialize settings system
            InitializeSettings();
            // Initialize custom notification system
            InitializeCustomNotifications();
            // Initialize audio system
            InitializeAudio();
            // Initialize input blocker with window handle for performance
            InitializeInputBlocker(hwnd);
            
            // Initialize productivity manager with main window
            g_productivityManager.SetMainWindow(hwnd);
            
            // Initialize privacy manager with main window
            g_privacyManager.SetMainWindow(hwnd);
            
            // Enable privacy features based on settings
            if (g_appSettings.bossKeyEnabled) {
                g_privacyManager.EnableBossKey(g_privacyManager.GetBossKeyModifiers(), g_privacyManager.GetBossKeyVirtualKey());
            }
            
            // Add the icon to the system tray on window creation
            AddTrayIcon(hwnd);
            // Show startup notification
            ShowNotification(hwnd, NOTIFY_APP_START);
            
            // Register hotkeys based on settings
            RegisterHotkeyFromSettings(hwnd);
            
            // Install the keyboard hook to listen for unlock sequence
            InstallHook();
            break;
        }

        case WM_HOTKEY:
            // Handle the global hotkey press
            if (wParam == HOTKEY_ID_LOCK) {
                ToggleInputLock(hwnd);
            } else if (wParam == HOTKEY_ID_UNLOCK) {
                // Regular unlock (Ctrl+O)
                if (IsInputLocked()) {
                    ToggleInputLock(hwnd);
                }
            } else if (wParam >= 5000 && wParam < 5100) {
                // Quick launch hotkeys (5000-5099 range)
                auto apps = g_productivityManager.GetQuickLaunchApps();
                int appIndex = wParam - 5000;
                if (appIndex < apps.size() && apps[appIndex].enabled) {
                    // Pass the hotkey ID directly
                    if (g_productivityManager.ExecuteQuickLaunchApp(wParam)) {
                        ShowNotification(hwnd, NOTIFY_APP_START, "Application launched successfully");
                    } else {
                        ShowNotification(hwnd, NOTIFY_HOTKEY_ERROR, "Failed to launch application");
                    }
                }
            } else if (wParam == 9001) {
                // Boss Key hotkey (registered by PrivacyManager)
                if (g_privacyManager.IsBossKeyActive()) {
                    g_privacyManager.DeactivateBossKey();
                } else {
                    g_privacyManager.ActivateBossKey();
                }
            }
            break;

        case WM_TRAY_ICON_MSG:
            // Handle messages from the system tray icon
            switch (lParam) {
                case WM_RBUTTONUP:
                    ShowContextMenu(hwnd);
                    break;
                case WM_LBUTTONDBLCLK:
                    // Double-click to toggle lock
                    ToggleInputLock(hwnd);
                    break;
            }
            break;

        case WM_COMMAND:
            // Handle context menu commands
            switch (LOWORD(wParam)) {
                case IDM_LOCK_UNLOCK:
                    ToggleInputLock(hwnd);
                    break;
                case IDM_SETTINGS:
                    ShowSettingsDialog(hwnd);
                    break;
                case IDM_CHANGE_HOTKEYS:
                    // Open settings dialog directly to Lock & Input tab
                    ShowSettingsDialog(hwnd);
                    break;
                case IDM_CHANGE_PASSWORD:
                    MessageBoxA(hwnd, "Password configuration coming soon!", "Change Password", MB_OK | MB_ICONINFORMATION);
                    break;
                case IDM_ABOUT:
                    MessageBoxA(hwnd, "UtilityApp v1.0\n\nHotkeys:\nLock: Ctrl+Shift+I\nUnlock: Ctrl+O or type '10203040'\nFailsafe: ESC x3 within 3 seconds\n\nIcon courtesy of Freepik (www.freepik.com)", "About", MB_OK | MB_ICONINFORMATION);
                    break;
                case IDM_EXIT:
                    ShowNotification(hwnd, NOTIFY_APP_EXIT);
                    DestroyWindow(hwnd);
                    break;
            }
            break;
            
        case WM_USER + 100:
            // Custom message: Deferred unlock operation from hook
            // This allows us to move expensive operations out of the hook procedure
            if (IsInputLocked()) {
                ToggleInputLock(hwnd); // This will unlock and show notification
                
                // CRITICAL: Clear stuck modifier keys by sending key release events
                // This fixes the issue where Ctrl/Shift remain "pressed" after unlock
                INPUT inputs[6] = {};
                int inputCount = 0;
                
                // Check if modifier keys are stuck and release them
                if (GetAsyncKeyState(VK_CONTROL) & 0x8000) {
                    inputs[inputCount].type = INPUT_KEYBOARD;
                    inputs[inputCount].ki.wVk = VK_CONTROL;
                    inputs[inputCount].ki.dwFlags = KEYEVENTF_KEYUP;
                    inputCount++;
                }
                if (GetAsyncKeyState(VK_SHIFT) & 0x8000) {
                    inputs[inputCount].type = INPUT_KEYBOARD;
                    inputs[inputCount].ki.wVk = VK_SHIFT;
                    inputs[inputCount].ki.dwFlags = KEYEVENTF_KEYUP;
                    inputCount++;
                }
                if (GetAsyncKeyState(VK_MENU) & 0x8000) {
                    inputs[inputCount].type = INPUT_KEYBOARD;
                    inputs[inputCount].ki.wVk = VK_MENU;
                    inputs[inputCount].ki.dwFlags = KEYEVENTF_KEYUP;
                    inputCount++;
                }
                
                // Send the key release events if any modifiers were stuck
                if (inputCount > 0) {
                    SendInput(inputCount, inputs, sizeof(INPUT));
                }
            }
            break;
            
        case WM_USER + 101: {
            // Custom message: Check custom password validation (deferred from hook)
            // wParam contains the current buffer length for optimization
            extern std::wstring g_passwordBuffer;
            if (IsInputLocked() && wParam >= 3) {
                // OPTIMIZATION: More efficient string conversion to reduce memory usage
                std::string currentInput;
                size_t bufferLen = std::min((size_t)wParam, (size_t)32); // Reasonable password limit
                currentInput.reserve(bufferLen + 1);
                
                // Convert wstring to string properly for password validation
                for (size_t i = 0; i < g_passwordBuffer.length(); i++) {
                    currentInput += (char)g_passwordBuffer[i];
                }
                
                // Check if this matches the custom password
                if (g_passwordManager.ValidatePassword(currentInput)) {
                    PostMessage(hwnd, WM_USER + 100, 0, 0); // Trigger unlock
                    g_passwordBuffer.clear();
                } else {
                    // Also check if the end of the buffer matches the password for rolling validation
                    if (currentInput.length() > 8) { // Only check rolling for longer inputs
                        for (size_t start = 1; start < currentInput.length() - 3; start++) {
                            std::string substring = currentInput.substr(start);
                            if (g_passwordManager.ValidatePassword(substring)) {
                                PostMessage(hwnd, WM_USER + 100, 0, 0); // Trigger unlock
                                g_passwordBuffer.clear();
                                break;
                            }
                        }
                    }
                }
            }
            break;
        }
        
        case WM_USER + 102: {
            // Deferred notification display to prevent input lag
            NotificationType type = (NotificationType)wParam;
            char* deferredMessage = (char*)lParam;
            
            const char* title = "UtilityApp";
            const char* message = deferredMessage ? deferredMessage : "Notification";
            DWORD iconType = NIIF_INFO;
            
            // Set appropriate icon based on type
            switch (type) {
                case NOTIFY_INPUT_LOCKED:
                case NOTIFY_FAILSAFE_TRIGGERED:
                    iconType = NIIF_WARNING;
                    break;
                case NOTIFY_HOTKEY_ERROR:
                    iconType = NIIF_ERROR;
                    break;
                default:
                    iconType = NIIF_INFO;
                    break;
            }
            
            // Display notification now that we're out of the hook context
            if (g_customNotifications) {
                g_customNotifications->ShowNotification(title, message);
            } else {
                extern void ShowBalloonTip(HWND hwnd, const char* title, const char* message, DWORD iconType);
                ShowBalloonTip(hwnd, title, message, iconType);
            }
            
            // Clean up dynamically allocated message
            if (deferredMessage) {
                free(deferredMessage);
            }
            break;
        }
        
        case WM_CLOSE:
            DestroyWindow(hwnd);
            break;

        case WM_DESTROY:
            // Cleanup resources before exiting
            RemoveTrayIcon(hwnd);
            UnregisterHotKey(hwnd, HOTKEY_ID_LOCK);
            UnregisterHotKey(hwnd, HOTKEY_ID_UNLOCK);
            UnregisterHotKey(hwnd, 9001); // Boss key hotkey
            UninstallHook();
            CleanupCustomNotifications();
            CleanupAudio();
            PostQuitMessage(0);
            break;
            
        case WM_DEVICECHANGE:
            // Handle USB device changes for productivity features
            g_productivityManager.HandleDeviceChange(wParam, lParam);
            break;
            
        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

// Entry Point
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Register the window class
    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME; // Using your ANSI class name
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APPICON));
    
    if (!RegisterClass(&wc)) {
        MessageBoxA(NULL, "Window Registration Failed!", "Error", MB_OK | MB_ICONERROR);
        return 0;
    }

    // Create a hidden window that can be controlled by privacy settings
    // This allows the Alt+Tab hiding feature to work properly
    HWND hwnd = CreateWindowEx(
        WS_EX_TOOLWINDOW,    // Tool window - hidden from taskbar by default
        CLASS_NAME,
        "UtilityApp",        // Window title (not visible)
        WS_OVERLAPPED,       // Normal window style but will be hidden
        CW_USEDEFAULT, CW_USEDEFAULT, 1, 1,  // Minimal size
        NULL,                // No parent window
        NULL,
        hInstance,
        NULL
    );

    if (hwnd == NULL) {
        MessageBoxA(NULL, "Window Creation Failed!", "Error", MB_OK | MB_ICONERROR);
        return 0;
    }
    
    // Store global reference for settings updates
    g_mainWindow = hwnd;
    
    // The ShowWindow(hwnd, SW_HIDE) call is no longer necessary,
    // as a message-only window is never shown.

    // Main message loop
    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}

// Function to register hotkeys based on current settings
void RegisterHotkeyFromSettings(HWND hwnd) {
    // Unregister existing hotkeys first
    UnregisterHotKey(hwnd, HOTKEY_ID_LOCK);
    UnregisterHotKey(hwnd, HOTKEY_ID_UNLOCK);
    
    // Register lock hotkey from settings
    if (!RegisterHotKey(hwnd, HOTKEY_ID_LOCK, g_appSettings.hotkeyModifiers, g_appSettings.hotkeyVirtualKey)) {
        MessageBoxA(hwnd, "Failed to register lock hotkey!", "Error", MB_OK | MB_ICONERROR);
        ShowNotification(hwnd, NOTIFY_HOTKEY_ERROR, "Failed to register lock hotkey");
    }
    
    // Register regular unlock hotkey (Ctrl+O)
    if (!RegisterHotKey(hwnd, HOTKEY_ID_UNLOCK, MOD_CONTROL, 'O')) {
        MessageBoxA(hwnd, "Failed to register unlock hotkey!", "Error", MB_OK | MB_ICONERROR);
        ShowNotification(hwnd, NOTIFY_HOTKEY_ERROR, "Failed to register unlock hotkey");
    }
}

// Utility function to parse hotkey strings (e.g., "Ctrl+Alt+F12")
bool ParseHotkeyString(const std::string& hotkeyStr, UINT& modifiers, UINT& virtualKey) {
    modifiers = 0;
    virtualKey = 0;
    
    if (hotkeyStr.empty()) return false;
    
    // Convert to uppercase for easier parsing
    std::string upperStr = hotkeyStr;
    std::transform(upperStr.begin(), upperStr.end(), upperStr.begin(), ::toupper);
    
    // Parse modifiers
    if (upperStr.find("CTRL") != std::string::npos) {
        modifiers |= MOD_CONTROL;
    }
    if (upperStr.find("ALT") != std::string::npos) {
        modifiers |= MOD_ALT;
    }
    if (upperStr.find("SHIFT") != std::string::npos) {
        modifiers |= MOD_SHIFT;
    }
    if (upperStr.find("WIN") != std::string::npos) {
        modifiers |= MOD_WIN;
    }
    
    // Find the main key (after the last '+')
    size_t lastPlus = upperStr.find_last_of('+');
    std::string keyStr;
    if (lastPlus != std::string::npos) {
        keyStr = upperStr.substr(lastPlus + 1);
    } else {
        keyStr = upperStr; // No modifiers, just the key
    }
    
    // Parse virtual key
    if (keyStr.length() == 1 && keyStr[0] >= 'A' && keyStr[0] <= 'Z') {
        virtualKey = keyStr[0]; // Letter key
    } else if (keyStr.length() == 1 && keyStr[0] >= '0' && keyStr[0] <= '9') {
        virtualKey = keyStr[0]; // Number key
    } else if (keyStr.substr(0, 1) == "F" && keyStr.length() >= 2) {
        // Function key (F1-F12)
        int funcNum = std::atoi(keyStr.substr(1).c_str());
        if (funcNum >= 1 && funcNum <= 12) {
            virtualKey = VK_F1 + (funcNum - 1);
        }
    } else {
        // Special keys
        if (keyStr == "ESC" || keyStr == "ESCAPE") {
            virtualKey = VK_ESCAPE;
        } else if (keyStr == "SPACE") {
            virtualKey = VK_SPACE;
        } else if (keyStr == "ENTER" || keyStr == "RETURN") {
            virtualKey = VK_RETURN;
        } else if (keyStr == "TAB") {
            virtualKey = VK_TAB;
        } else if (keyStr == "BACKSPACE") {
            virtualKey = VK_BACK;
        } else if (keyStr == "DELETE" || keyStr == "DEL") {
            virtualKey = VK_DELETE;
        } else if (keyStr == "INSERT" || keyStr == "INS") {
            virtualKey = VK_INSERT;
        } else if (keyStr == "HOME") {
            virtualKey = VK_HOME;
        } else if (keyStr == "END") {
            virtualKey = VK_END;
        } else if (keyStr == "PAGE_UP" || keyStr == "PGUP") {
            virtualKey = VK_PRIOR;
        } else if (keyStr == "PAGE_DOWN" || keyStr == "PGDN") {
            virtualKey = VK_NEXT;
        } else {
            return false; // Unrecognized key
        }
    }
    
    return virtualKey != 0;
}