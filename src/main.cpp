// src/main.cpp

#include <windows.h>
#include <string>
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
#include "features/lock_input/password_manager.h"

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
            // Initialize settings system FIRST - before any notifications
            InitializeSettings();
            
            // Initialize custom notification system AFTER settings are loaded
            InitializeCustomNotifications();
            
            // Initialize audio system
            InitializeAudio();
            
            // Initialize input blocker with window handle for performance
            InitializeInputBlocker(hwnd);
            
            // Initialize productivity manager with main window
            g_productivityManager.SetMainWindow(hwnd);
            
            // Initialize privacy manager with main window
            g_privacyManager.SetMainWindow(hwnd);
            
            // Apply all loaded settings to the feature managers
            g_settingsCore.ApplySettings(g_appSettings, hwnd);
            
            // Add the icon to the system tray on window creation
            AddTrayIcon(hwnd);
            
            // Show startup notification (settings are now loaded)
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
            NotificationLevel level = NOTIFY_LEVEL_INFO;
            
            // Set appropriate icon and level based on type
            switch (type) {
                case NOTIFY_INPUT_LOCKED:
                case NOTIFY_FAILSAFE_TRIGGERED:
                    iconType = NIIF_WARNING;
                    level = NOTIFY_LEVEL_WARNING;
                    break;
                case NOTIFY_HOTKEY_ERROR:
                case NOTIFY_SETTINGS_ERROR:
                    iconType = NIIF_ERROR;
                    level = NOTIFY_LEVEL_ERROR;
                    break;
                default:
                    iconType = NIIF_INFO;
                    level = NOTIFY_LEVEL_INFO;
                    break;
            }
            
            // Display notification now that we're out of the hook context
            if (g_customNotifications) {
                g_customNotifications->ShowNotification(title, message, 4000, level);
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
    
    // Use const reference to avoid copy
    const std::string& str = hotkeyStr;
    size_t len = str.length();
    
    // Parse modifiers using more efficient string search
    size_t pos = 0;
    while (pos < len) {
        if (str[pos] == 'C' && pos + 3 < len && 
            str[pos+1] == 't' && str[pos+2] == 'r' && str[pos+3] == 'l') {
            modifiers |= MOD_CONTROL;
            pos += 4;
            if (pos < len && str[pos] == '+') pos++;
        } else if (str[pos] == 'A' && pos + 2 < len && 
                   str[pos+1] == 'l' && str[pos+2] == 't') {
            modifiers |= MOD_ALT;
            pos += 3;
            if (pos < len && str[pos] == '+') pos++;
        } else if (str[pos] == 'S' && pos + 4 < len && 
                   str[pos+1] == 'h' && str[pos+2] == 'i' && str[pos+3] == 'f' && str[pos+4] == 't') {
            modifiers |= MOD_SHIFT;
            pos += 5;
            if (pos < len && str[pos] == '+') pos++;
        } else if (str[pos] == 'W' && pos + 2 < len && 
                   str[pos+1] == 'i' && str[pos+2] == 'n') {
            modifiers |= MOD_WIN;
            pos += 3;
            if (pos < len && str[pos] == '+') pos++;
        } else {
            break; // Found the main key
        }
    }
    
    // Parse the main key from the remaining string
    if (pos >= len) return false;
    
    std::string keyStr = str.substr(pos);
    
    // Optimize key parsing with direct character checks
    if (keyStr.length() == 1) {
        char ch = keyStr[0];
        if ((ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9')) {
            virtualKey = ch;
            return true;
        }
    } else if (keyStr[0] == 'F' && keyStr.length() >= 2 && keyStr.length() <= 3) {
        // Function key (F1-F12)
        int funcNum = 0;
        if (keyStr.length() == 2) {
            funcNum = keyStr[1] - '0';
        } else {
            funcNum = (keyStr[1] - '0') * 10 + (keyStr[2] - '0');
        }
        if (funcNum >= 1 && funcNum <= 12) {
            virtualKey = VK_F1 + (funcNum - 1);
            return true;
        }
    }
    
    // Special keys - use a more efficient lookup
    static const struct {
        const char* name;
        UINT vk;
    } specialKeys[] = {
        {"ESC", VK_ESCAPE}, {"ESCAPE", VK_ESCAPE},
        {"SPACE", VK_SPACE}, {"ENTER", VK_RETURN}, {"RETURN", VK_RETURN},
        {"TAB", VK_TAB}, {"BACKSPACE", VK_BACK}, {"DELETE", VK_DELETE},
        {"DEL", VK_DELETE}, {"INSERT", VK_INSERT}, {"INS", VK_INSERT},
        {"HOME", VK_HOME}, {"END", VK_END},
        {"PAGEUP", VK_PRIOR}, {"PAGEDOWN", VK_NEXT},
        {"PGUP", VK_PRIOR}, {"PGDN", VK_NEXT}
    };
    
    for (const auto& key : specialKeys) {
        if (keyStr == key.name) {
            virtualKey = key.vk;
            return true;
        }
    }
    
    return false;
}