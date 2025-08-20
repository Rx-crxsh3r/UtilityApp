// src/main.cpp

#include <windows.h>
#include "resource.h"
#include "tray_icon.h"
#include "input_blocker.h"
#include "failsafe.h"
#include "notifications.h"

// Global variables
extern const char CLASS_NAME[] = "UtilityAppClass";
Failsafe failsafeHandler;

// Window Procedure
LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE:
            // Add the icon to the system tray on window creation
            AddTrayIcon(hwnd);
            // Show startup notification
            ShowNotification(hwnd, NOTIFY_APP_START);
            // Register the global hotkeys
            // Lock: Ctrl + Shift + Esc + I (simplified to Ctrl + Shift + I for now)
            if (!RegisterHotKey(hwnd, HOTKEY_ID_LOCK, MOD_CONTROL | MOD_SHIFT, 'I')) {
                 MessageBoxA(hwnd, "Failed to register lock hotkey!", "Error", MB_OK | MB_ICONERROR);
                 ShowNotification(hwnd, NOTIFY_HOTKEY_ERROR, "Failed to register lock hotkey");
            }
            // Unlock: Ctrl + Esc + O (simplified to Ctrl + O for now)
            if (!RegisterHotKey(hwnd, HOTKEY_ID_UNLOCK, MOD_CONTROL, 'O')) {
                 MessageBoxA(hwnd, "Failed to register unlock hotkey!", "Error", MB_OK | MB_ICONERROR);
                 ShowNotification(hwnd, NOTIFY_HOTKEY_ERROR, "Failed to register unlock hotkey");
            }
            // Install the keyboard hook to listen for unlock sequence
            InstallHook();
            break;

        case WM_HOTKEY:
            // Handle the global hotkey press
            if (wParam == HOTKEY_ID_LOCK) {
                ToggleInputLock(hwnd);
            } else if (wParam == HOTKEY_ID_UNLOCK) {
                // Force unlock
                if (IsInputLocked()) {
                    ToggleInputLock(hwnd);
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
                    MessageBoxA(hwnd, "Settings dialog coming soon!", "Settings", MB_OK | MB_ICONINFORMATION);
                    break;
                case IDM_CHANGE_HOTKEYS:
                    MessageBoxA(hwnd, "Hotkey configuration coming soon!", "Change Hotkeys", MB_OK | MB_ICONINFORMATION);
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
            
        case WM_CLOSE:
            DestroyWindow(hwnd);
            break;

        case WM_DESTROY:
            // Cleanup resources before exiting
            RemoveTrayIcon(hwnd);
            UnregisterHotKey(hwnd, HOTKEY_ID_LOCK);
            UnregisterHotKey(hwnd, HOTKEY_ID_UNLOCK);
            UninstallHook();
            PostQuitMessage(0);
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
    wc.lpszClassName = CLASS_NAME;
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APPICON));
    
    if (!RegisterClass(&wc)) {
        MessageBoxA(NULL, "Window Registration Failed!", "Error", MB_OK | MB_ICONERROR);
        return 0;
    }

    // Create a hidden window to handle messages
    HWND hwnd = CreateWindowEx(
        0, CLASS_NAME, "UtilityApp", 
        0, // No window style - completely hidden
        CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, // No size
        HWND_MESSAGE, // This creates a message-only window that doesn't appear anywhere
        NULL, hInstance, NULL
    );

    if (hwnd == NULL) {
        MessageBoxA(NULL, "Window Creation Failed!", "Error", MB_OK | MB_ICONERROR);
        return 0;
    }
    
    // Main message loop
    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}