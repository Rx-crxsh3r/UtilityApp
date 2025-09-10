// src/features/lock_input/lock_input_tab.cpp
// Lock Input Tab - OOP implementation

#include "lock_input_tab.h"
#include "../../settings.h"  // For SettingsDialog access
#include "../../settings/settings_core.h"
#include "hotkey_manager.h"
#include "../../resource.h"  // For control IDs
#include <commctrl.h>

// External references
extern HotkeyManager g_hotkeyManager;

// Static callback for Windows dialog system
INT_PTR CALLBACK LockInputTab::DialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    LockInputTab* tab = nullptr;

    if (message == WM_INITDIALOG) {
        tab = (LockInputTab*)lParam;
        SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)tab);
        tab->SetDialogHandle(hDlg);
    } else {
        tab = (LockInputTab*)GetWindowLongPtr(hDlg, GWLP_USERDATA);
    }

    if (tab) {
        return tab->HandleMessage(hDlg, message, wParam, lParam);
    }

    return FALSE;
}

LockInputTab::LockInputTab(SettingsDialog* parent, AppSettings* settings, bool* unsavedFlag)
    : parentDialog(parent), tempSettings(settings), hasUnsavedChanges(unsavedFlag),
      hTabDialog(nullptr), hWarningKeyboardUnlock(nullptr),
      hWarningLockingDisabled(nullptr), hWarningSingleKey(nullptr) {
}

LockInputTab::~LockInputTab() {
    // Clean up warning controls
    if (hWarningKeyboardUnlock) DestroyWindow(hWarningKeyboardUnlock);
    if (hWarningLockingDisabled) DestroyWindow(hWarningLockingDisabled);
    if (hWarningSingleKey) DestroyWindow(hWarningSingleKey);
}

INT_PTR LockInputTab::HandleMessage(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_INITDIALOG: {
            hTabDialog = hDlg;
            InitializeControls();
            return TRUE;
        }

        case WM_USER + 101: {
            // Custom message from hotkey manager - hotkey capture completed
            // Read the updated hotkey from the text box and update tempSettings
            char hotkeyBuffer[256];
            GetDlgItemTextA(hTabDialog, IDC_EDIT_HOTKEY_LOCK, hotkeyBuffer, sizeof(hotkeyBuffer));
            std::string newHotkey = std::string(hotkeyBuffer);
            
            // Check if hotkey actually changed
            if (newHotkey != tempSettings->lockHotkey) {
                tempSettings->lockHotkey = newHotkey;
                
                // Parse hotkey to extract modifiers and virtual key
                extern bool ParseHotkeyString(const std::string& hotkeyStr, UINT& modifiers, UINT& virtualKey);
                ParseHotkeyString(tempSettings->lockHotkey, 
                                (UINT&)tempSettings->hotkeyModifiers, 
                                (UINT&)tempSettings->hotkeyVirtualKey);
                
                // Mark as having unsaved changes and update Apply button
                *hasUnsavedChanges = true;
                if (parentDialog) {
                    parentDialog->UpdateButtonStates();
                }
            }
            
            // Update warnings with new hotkey
            UpdateWarnings();
            return TRUE;
        }

        case WM_CTLCOLORSTATIC: {
            // Make warning labels red
            HWND hControl = (HWND)lParam;
            int controlId = GetDlgCtrlID(hControl);

            if (controlId == IDC_WARNING_KEYBOARD_UNLOCK ||
                controlId == IDC_WARNING_LOCKING_DISABLED ||
                controlId == IDC_WARNING_SINGLE_KEY) {

                HDC hdc = (HDC)wParam;
                SetTextColor(hdc, RGB(255, 0, 0)); // Red text
                SetBkMode(hdc, TRANSPARENT);       // Transparent background
                return (INT_PTR)GetStockObject(NULL_BRUSH);
            }
            break;
        }

        case WM_COMMAND: {
            HandleControlCommand(wParam, lParam);
            break;
        }
    }

    return FALSE;
}

void LockInputTab::InitializeControls() {
    if (!hTabDialog || !tempSettings) return;

    // Initialize controls with current settings
    CheckDlgButton(hTabDialog, IDC_CHECK_KEYBOARD, tempSettings->keyboardLockEnabled ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hTabDialog, IDC_CHECK_MOUSE, tempSettings->mouseLockEnabled ? BST_CHECKED : BST_UNCHECKED);

    // Set unlock method radio buttons (only Password and Timer now)
    CheckRadioButton(hTabDialog, IDC_RADIO_PASSWORD, IDC_RADIO_TIMER,
                   IDC_RADIO_PASSWORD + tempSettings->unlockMethod);

    // Set whitelist checkbox separately (it's now an addon feature)
    CheckDlgButton(hTabDialog, IDC_CHECK_WHITELIST, tempSettings->whitelistEnabled ? BST_CHECKED : BST_UNCHECKED);
    EnableWindow(GetDlgItem(hTabDialog, IDC_BTN_WHITELIST_CFG), tempSettings->whitelistEnabled);

    // Set hotkey text
    SetDlgItemTextA(hTabDialog, IDC_EDIT_HOTKEY_LOCK, tempSettings->lockHotkey.c_str());

    // Create warning controls dynamically
    CreateWarningControls();

    // Initialize warnings
    UpdateWarnings();
}

void LockInputTab::HandleControlCommand(WPARAM wParam, LPARAM lParam) {
    switch (LOWORD(wParam)) {
        case IDC_CHECK_KEYBOARD:
        case IDC_CHECK_MOUSE: {
            bool oldKeyboard = tempSettings->keyboardLockEnabled;
            bool oldMouse = tempSettings->mouseLockEnabled;

            tempSettings->keyboardLockEnabled = IsDlgButtonChecked(hTabDialog, IDC_CHECK_KEYBOARD) == BST_CHECKED;
            tempSettings->mouseLockEnabled = IsDlgButtonChecked(hTabDialog, IDC_CHECK_MOUSE) == BST_CHECKED;

            // Check if this created a pending change
            if (oldKeyboard != tempSettings->keyboardLockEnabled ||
                oldMouse != tempSettings->mouseLockEnabled) {
                *hasUnsavedChanges = true;
                // Notify parent dialog to update button states
                if (parentDialog) {
                    parentDialog->UpdateButtonStates();
                }
            }

            // Update warnings when lock type changes
            UpdateWarnings();
            break;
        }

        case IDC_RADIO_PASSWORD:
        case IDC_RADIO_TIMER: {
            int oldMethod = tempSettings->unlockMethod;
            tempSettings->unlockMethod = LOWORD(wParam) - IDC_RADIO_PASSWORD;

            if (oldMethod != tempSettings->unlockMethod) {
                *hasUnsavedChanges = true;
                // Notify parent dialog to update button states
                if (parentDialog) {
                    parentDialog->UpdateButtonStates();
                }
            }

            // Update warnings when unlock method changes
            UpdateWarnings();
            break;
        }

        case IDC_CHECK_WHITELIST: {
            bool oldValue = tempSettings->whitelistEnabled;
            tempSettings->whitelistEnabled = (IsDlgButtonChecked(hTabDialog, IDC_CHECK_WHITELIST) == BST_CHECKED);

            // Enable/disable whitelist configuration button
            EnableWindow(GetDlgItem(hTabDialog, IDC_BTN_WHITELIST_CFG), tempSettings->whitelistEnabled);

            if (oldValue != tempSettings->whitelistEnabled) {
                *hasUnsavedChanges = true;
                // Notify parent dialog to update button states
                if (parentDialog) {
                    parentDialog->UpdateButtonStates();
                }
            }
            break;
        }

        case IDC_BTN_PASSWORD_CFG:
            ShowPasswordConfig();
            break;

        case IDC_BTN_TIMER_CFG:
            ShowTimerConfig();
            break;

        case IDC_BTN_WHITELIST_CFG:
            ShowWhitelistConfig();
            break;

        case IDC_EDIT_HOTKEY_LOCK:
            if (HIWORD(wParam) == EN_SETFOCUS) {
                // User clicked textbox - start capture using modular system
                HWND hEdit = GetDlgItem(hTabDialog, IDC_EDIT_HOTKEY_LOCK);
                HWND hHint = GetDlgItem(hTabDialog, IDC_LABEL_HOTKEY_HINT);
                g_hotkeyManager.StartCapture(hTabDialog, hEdit, hHint, tempSettings->lockHotkey);
            }
            break;
    }
}

void LockInputTab::UpdateWarnings() {
    if (!hTabDialog) return;

    // Get current settings from UI
    bool keyboardEnabled = IsDlgButtonChecked(hTabDialog, IDC_CHECK_KEYBOARD) == BST_CHECKED;
    bool mouseEnabled = IsDlgButtonChecked(hTabDialog, IDC_CHECK_MOUSE) == BST_CHECKED;
    bool passwordSelected = IsDlgButtonChecked(hTabDialog, IDC_RADIO_PASSWORD) == BST_CHECKED;

    // Get hotkey text and check if it's a single key
    char hotkeyBuffer[256];
    GetDlgItemTextA(hTabDialog, IDC_EDIT_HOTKEY_LOCK, hotkeyBuffer, sizeof(hotkeyBuffer));
    std::string currentHotkey = std::string(hotkeyBuffer);
    bool isSingleKey = g_hotkeyManager.IsSingleKey(currentHotkey);

    // Warning 1: Password won't work if keyboard is unlocked
    if (hWarningKeyboardUnlock && !keyboardEnabled && passwordSelected) {
        SetWindowTextA(hWarningKeyboardUnlock, "!!WARNING!!: Password unlock will not work with keyboard unlocked.");
        ShowWindow(hWarningKeyboardUnlock, SW_SHOW);
    } else if (hWarningKeyboardUnlock) {
        ShowWindow(hWarningKeyboardUnlock, SW_HIDE);
    }

    // Warning 2: Locking mechanism disabled
    if (hWarningLockingDisabled && !keyboardEnabled && !mouseEnabled) {
        SetWindowTextA(hWarningLockingDisabled, "!!WARNING!!: Locking mechanism will be disabled.");
        ShowWindow(hWarningLockingDisabled, SW_SHOW);
    } else if (hWarningLockingDisabled) {
        ShowWindow(hWarningLockingDisabled, SW_HIDE);
    }

    // Warning 3: Single key hotkey
    if (hWarningSingleKey && isSingleKey && !currentHotkey.empty()) {
        SetWindowTextA(hWarningSingleKey, "!!WARNING!!: Single letter hotkeys are not recommended for security.");
        ShowWindow(hWarningSingleKey, SW_SHOW);
    } else if (hWarningSingleKey) {
        ShowWindow(hWarningSingleKey, SW_HIDE);
    }
}

void LockInputTab::CreateWarningControls() {
    if (!hTabDialog) return;

    // Get dialog font for consistent styling
    HFONT hDialogFont = (HFONT)SendMessage(hTabDialog, WM_GETFONT, 0, 0);

    // Create Warning 1: Password won't work with keyboard unlocked
    hWarningKeyboardUnlock = CreateWindowA("STATIC", "",
        WS_CHILD | SS_LEFT,
        20, 280, 360, 40,  // x, y, width, height (increased height for 2 lines)
        hTabDialog, (HMENU)IDC_WARNING_KEYBOARD_UNLOCK, GetModuleHandle(NULL), NULL);
    if (hWarningKeyboardUnlock) {
        SendMessage(hWarningKeyboardUnlock, WM_SETFONT, (WPARAM)hDialogFont, TRUE);
    }

    // Create Warning 2: Locking mechanism disabled
    hWarningLockingDisabled = CreateWindowA("STATIC", "",
        WS_CHILD | SS_LEFT,
        20, 320, 360, 30,  // x, y, width, height
        hTabDialog, (HMENU)IDC_WARNING_LOCKING_DISABLED, GetModuleHandle(NULL), NULL);
    if (hWarningLockingDisabled) {
        SendMessage(hWarningLockingDisabled, WM_SETFONT, (WPARAM)hDialogFont, TRUE);
    }

    // Create Warning 3: Single key hotkey warning
    hWarningSingleKey = CreateWindowA("STATIC", "",
        WS_CHILD | SS_LEFT,
        20, 360, 360, 30,  // x, y, width, height
        hTabDialog, (HMENU)IDC_WARNING_SINGLE_KEY, GetModuleHandle(NULL), NULL);
    if (hWarningSingleKey) {
        SendMessage(hWarningSingleKey, WM_SETFONT, (WPARAM)hDialogFont, TRUE);
    }
}

void LockInputTab::RefreshControls() {
    if (hTabDialog) {
        // Re-initialize the tab with current tempSettings values
        InitializeControls();
    }
}

void LockInputTab::ShowPasswordConfig() {
    if (parentDialog) {
        // Delegate to parent dialog's method
        MessageBoxA(hTabDialog, "Password configuration dialog coming soon!", "Password Settings", MB_OK | MB_ICONINFORMATION);
    }
}

void LockInputTab::ShowTimerConfig() {
    if (parentDialog) {
        // Delegate to parent dialog's method
        MessageBoxA(hTabDialog, "Timer configuration dialog coming soon!", "Timer Settings", MB_OK | MB_ICONINFORMATION);
    }
}

void LockInputTab::ShowWhitelistConfig() {
    if (parentDialog) {
        // Delegate to parent dialog's method
        MessageBoxA(hTabDialog, "Whitelist configuration dialog coming soon!", "Whitelist Settings", MB_OK | MB_ICONINFORMATION);
    }
}
