// src/settings.cpp
// Settings system implementation - using modular components

#include "settings.h"
#include "resource.h"
#include "input_blocker.h"
#include "features/privacy/privacy_manager.h"
#include "features/productivity/productivity_manager.h"
#include "custom_notifications.h"
#include <commctrl.h>
#include <fstream>
#include <sstream>
#include <map>

// Global settings instance
AppSettings g_appSettings;

// Global manager instances
extern ProductivityManager g_productivityManager;
extern PrivacyManager g_privacyManager;

// Static pointer for dialog procedures
static SettingsDialog* g_currentDialog = nullptr;

SettingsDialog::SettingsDialog(AppSettings* appSettings) 
    : hMainDialog(nullptr), hTabControl(nullptr), hCurrentTab(nullptr),
      currentTabIndex(0), settings(appSettings), hasUnsavedChanges(false),
      isEditingHotkey(false), hTabLockInput(nullptr), hTabProductivity(nullptr),
      hTabPrivacy(nullptr), hTabAppearance(nullptr), isCapturingHotkey(false),
      ctrlPressed(false), shiftPressed(false), altPressed(false), winPressed(false),
      hKeyboardHook(nullptr) {
    
    // Try to load from registry first, if that fails, use passed settings or defaults
    if (!g_settingsCore.LoadSettings(tempSettings)) {
        if (appSettings) {
            tempSettings = *appSettings; // Use passed settings
        } else {
            tempSettings = AppSettings(); // Use defaults
        }
    }
    
    // Update the passed settings with loaded values
    if (appSettings) {
        *appSettings = tempSettings;
    }
    
    originalSettings = tempSettings; // Store original for change detection
}

SettingsDialog::~SettingsDialog() {
    if (hKeyboardHook) {
        UnhookWindowsHookEx(hKeyboardHook);
    }
    if (hTabLockInput) DestroyWindow(hTabLockInput);
    if (hTabProductivity) DestroyWindow(hTabProductivity);
    if (hTabPrivacy) DestroyWindow(hTabPrivacy);
    if (hTabAppearance) DestroyWindow(hTabAppearance);
}

bool SettingsDialog::ShowDialog(HWND parent) {
    g_currentDialog = this;
    
    // Create main settings dialog (we'll define this in resource file)
    INT_PTR result = DialogBoxParam(GetModuleHandle(NULL), 
                                   MAKEINTRESOURCE(IDD_SETTINGS_DIALOG),
                                   parent, DialogProc, (LPARAM)this);
    
    g_currentDialog = nullptr;
    return (result == IDOK);
}

INT_PTR CALLBACK SettingsDialog::DialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    SettingsDialog* dialog = nullptr;
    
    if (message == WM_INITDIALOG) {
        dialog = (SettingsDialog*)lParam;
        SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)dialog);
        dialog->hMainDialog = hDlg;
    } else {
        dialog = (SettingsDialog*)GetWindowLongPtr(hDlg, GWLP_USERDATA);
    }
    
    if (!dialog) return FALSE;
    
    switch (message) {
        case WM_INITDIALOG: {
            // Initialize tab control
            dialog->hTabControl = GetDlgItem(hDlg, IDC_TAB_CONTROL);
            
            // Add tabs
            TCITEM tie = {};
            tie.mask = TCIF_TEXT;
            
            tie.pszText = (LPSTR)"Lock & Input";
            TabCtrl_InsertItem(dialog->hTabControl, 0, &tie);
            
            tie.pszText = (LPSTR)"Productivity";
            TabCtrl_InsertItem(dialog->hTabControl, 1, &tie);
            
            tie.pszText = (LPSTR)"Privacy & Security";
            TabCtrl_InsertItem(dialog->hTabControl, 2, &tie);
            
            tie.pszText = (LPSTR)"Appearance";
            TabCtrl_InsertItem(dialog->hTabControl, 3, &tie);
            
            // Create tab dialogs
            dialog->CreateTabDialogs();
            dialog->SwitchTab(0); // Show first tab
            dialog->LoadSettings();
            
            return TRUE;
        }
        
        case WM_NOTIFY: {
            LPNMHDR pnmh = (LPNMHDR)lParam;
            if (pnmh->idFrom == IDC_TAB_CONTROL && pnmh->code == TCN_SELCHANGE) {
                int newTab = TabCtrl_GetCurSel(dialog->hTabControl);
                if (newTab != dialog->currentTabIndex) {
                    if (dialog->HasPendingChanges()) {
                        int result = MessageBoxA(dialog->hMainDialog,
                                               "You have unsaved changes. Do you want to save them?",
                                               "Unsaved Changes", 
                                               MB_YESNOCANCEL | MB_ICONQUESTION);
                        switch (result) {
                            case IDYES:
                                dialog->SaveSettings();
                                dialog->ApplySettings();
                                dialog->hasUnsavedChanges = false;
                                dialog->originalSettings = dialog->tempSettings;
                                break;
                            case IDNO:
                                dialog->tempSettings = dialog->originalSettings; // Revert changes
                                dialog->hasUnsavedChanges = false;
                                dialog->RefreshCurrentTabControls(); // Refresh UI to show reverted values
                                break;
                            case IDCANCEL:
                                // Revert tab selection
                                TabCtrl_SetCurSel(dialog->hTabControl, dialog->currentTabIndex);
                                return TRUE;
                        }
                    }
                    dialog->SwitchTab(newTab);
                }
            }
            break;
        }
        
        case WM_COMMAND: {
            switch (LOWORD(wParam)) {
                case IDC_BTN_OK:
                    // OK = Discard changes (with confirmation if pending)
                    if (dialog->HasPendingChanges()) {
                        int result = MessageBoxA(dialog->hMainDialog,
                                                "You have unsaved changes. Are you sure you want to discard them?",
                                                "Discard Changes?", 
                                                MB_YESNO | MB_ICONQUESTION);
                        if (result == IDNO) {
                            return TRUE; // Don't close
                        }
                    }
                    EndDialog(hDlg, IDOK);
                    return TRUE;
                    
                case IDC_BTN_CANCEL:
                    // Cancel = Close directly without confirmation
                    EndDialog(hDlg, IDCANCEL);
                    return TRUE;
                    
                case IDC_BTN_APPLY:
                    // Apply = Save all changes
                    dialog->SaveSettings();
                    dialog->ApplySettings();
                    dialog->hasUnsavedChanges = false;
                    dialog->originalSettings = dialog->tempSettings; // Update baseline
                    return TRUE;
            }
            break;
        }
        
        case WM_CLOSE:
            if (dialog->HasPendingChanges()) {
                int result = MessageBoxA(dialog->hMainDialog,
                                       "You have unsaved changes. Are you sure you want to discard them?",
                                       "Discard Changes?", 
                                       MB_YESNO | MB_ICONQUESTION);
                if (result == IDNO) {
                    return TRUE; // Don't close
                }
            }
            EndDialog(hDlg, IDCANCEL);
            return TRUE;
    }
    
    return FALSE;
}

void SettingsDialog::CreateTabDialogs() {
    RECT rcTab;
    GetWindowRect(hTabControl, &rcTab);
    ScreenToClient(hMainDialog, (POINT*)&rcTab.left);
    ScreenToClient(hMainDialog, (POINT*)&rcTab.right);
    
    // Adjust for tab control borders
    TabCtrl_AdjustRect(hTabControl, FALSE, &rcTab);
    
    // Create Lock & Input tab (modeless dialog)
    hTabLockInput = CreateDialogParam(GetModuleHandle(NULL),
                                     MAKEINTRESOURCE(IDD_TAB_LOCK_INPUT),
                                     hMainDialog, LockInputTabProc, (LPARAM)this);
    
    // Position the tab dialog
    if (hTabLockInput) {
        SetWindowPos(hTabLockInput, NULL, rcTab.left, rcTab.top,
                    rcTab.right - rcTab.left, rcTab.bottom - rcTab.top,
                    SWP_NOZORDER);
    }
    
    // Create other tabs (for now, simple message dialogs)
    hTabProductivity = CreateDialogParam(GetModuleHandle(NULL),
                                        MAKEINTRESOURCE(IDD_TAB_PRODUCTIVITY),
                                        hMainDialog, ProductivityTabProc, (LPARAM)this);
    if (hTabProductivity) {
        SetWindowPos(hTabProductivity, NULL, rcTab.left, rcTab.top,
                    rcTab.right - rcTab.left, rcTab.bottom - rcTab.top,
                    SWP_NOZORDER);
    }
    
    hTabPrivacy = CreateDialogParam(GetModuleHandle(NULL),
                                   MAKEINTRESOURCE(IDD_TAB_PRIVACY),
                                   hMainDialog, PrivacyTabProc, (LPARAM)this);
    if (hTabPrivacy) {
        SetWindowPos(hTabPrivacy, NULL, rcTab.left, rcTab.top,
                    rcTab.right - rcTab.left, rcTab.bottom - rcTab.top,
                    SWP_NOZORDER);
    }
    
    hTabAppearance = CreateDialogParam(GetModuleHandle(NULL),
                                      MAKEINTRESOURCE(IDD_TAB_APPEARANCE),
                                      hMainDialog, AppearanceTabProc, (LPARAM)this);
    if (hTabAppearance) {
        SetWindowPos(hTabAppearance, NULL, rcTab.left, rcTab.top,
                    rcTab.right - rcTab.left, rcTab.bottom - rcTab.top,
                    SWP_NOZORDER);
    }
}

void SettingsDialog::SwitchTab(int tabIndex) {
    // End any active hotkey capture when switching tabs
    extern HotkeyManager g_hotkeyManager;
    g_hotkeyManager.EndCapture(false); // Don't save the hotkey
    
    HideCurrentTab();
    currentTabIndex = tabIndex;
    
    switch (tabIndex) {
        case TAB_LOCK_INPUT:
            ShowTabDialog(hTabLockInput);
            break;
        case TAB_PRODUCTIVITY:
            ShowTabDialog(hTabProductivity);
            break;
        case TAB_PRIVACY:
            ShowTabDialog(hTabPrivacy);
            break;
        case TAB_APPEARANCE:
            ShowTabDialog(hTabAppearance);
            break;
    }
}

void SettingsDialog::ShowTabDialog(HWND hTab) {
    if (hTab) {
        hCurrentTab = hTab;
        ShowWindow(hTab, SW_SHOW);
    }
}

void SettingsDialog::HideCurrentTab() {
    if (hCurrentTab) {
        ShowWindow(hCurrentTab, SW_HIDE);
        hCurrentTab = nullptr;
    }
}

void SettingsDialog::RefreshCurrentTabControls() {
    // Send WM_INITDIALOG to the current tab to refresh its controls
    // This effectively re-initializes the tab with current tempSettings values
    switch (currentTabIndex) {
        case TAB_LOCK_INPUT:
            if (hTabLockInput) {
                LockInputTabProc(hTabLockInput, WM_INITDIALOG, 0, (LPARAM)this);
            }
            break;
        case TAB_PRODUCTIVITY:
            if (hTabProductivity) {
                ProductivityTabProc(hTabProductivity, WM_INITDIALOG, 0, (LPARAM)this);
            }
            break;
        case TAB_PRIVACY:
            if (hTabPrivacy) {
                PrivacyTabProc(hTabPrivacy, WM_INITDIALOG, 0, (LPARAM)this);
            }
            break;
        case TAB_APPEARANCE:
            if (hTabAppearance) {
                AppearanceTabProc(hTabAppearance, WM_INITDIALOG, 0, (LPARAM)this);
            }
            break;
    }
}

INT_PTR CALLBACK SettingsDialog::LockInputTabProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    SettingsDialog* dialog = nullptr;
    
    if (message == WM_INITDIALOG) {
        dialog = (SettingsDialog*)lParam;
        SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)dialog);
    } else {
        dialog = (SettingsDialog*)GetWindowLongPtr(hDlg, GWLP_USERDATA);
    }
    
    if (!dialog) return FALSE;
    
    switch (message) {
        case WM_INITDIALOG: {
            // Initialize controls with current settings
            CheckDlgButton(hDlg, IDC_CHECK_KEYBOARD, dialog->tempSettings.keyboardLockEnabled ? BST_CHECKED : BST_UNCHECKED);
            CheckDlgButton(hDlg, IDC_CHECK_MOUSE, dialog->tempSettings.mouseLockEnabled ? BST_CHECKED : BST_UNCHECKED);
            
            // Set unlock method radio buttons (only Password and Timer now)
            CheckRadioButton(hDlg, IDC_RADIO_PASSWORD, IDC_RADIO_TIMER, 
                           IDC_RADIO_PASSWORD + dialog->tempSettings.unlockMethod);
            
            // Set whitelist checkbox separately (it's now an addon feature)
            CheckDlgButton(hDlg, IDC_CHECK_WHITELIST, dialog->tempSettings.whitelistEnabled ? BST_CHECKED : BST_UNCHECKED);
            EnableWindow(GetDlgItem(hDlg, IDC_BTN_WHITELIST_CFG), dialog->tempSettings.whitelistEnabled);
            
            // Set hotkey text
            SetDlgItemTextA(hDlg, IDC_EDIT_HOTKEY_LOCK, dialog->tempSettings.lockHotkey.c_str());
            
            // Create warning controls dynamically
            dialog->CreateWarningControls(hDlg);
            
            // Initialize warnings
            dialog->UpdateWarnings();
            
            return TRUE;
        }
        
        case WM_USER + 101: {
            // Custom message from hotkey manager - update warnings
            dialog->UpdateWarnings();
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
            switch (LOWORD(wParam)) {
                case IDC_CHECK_KEYBOARD:
                case IDC_CHECK_MOUSE: {
                    bool oldKeyboard = dialog->tempSettings.keyboardLockEnabled;
                    bool oldMouse = dialog->tempSettings.mouseLockEnabled;
                    
                    dialog->tempSettings.keyboardLockEnabled = IsDlgButtonChecked(hDlg, IDC_CHECK_KEYBOARD) == BST_CHECKED;
                    dialog->tempSettings.mouseLockEnabled = IsDlgButtonChecked(hDlg, IDC_CHECK_MOUSE) == BST_CHECKED;
                    
                    // Check if this created a pending change
                    if (oldKeyboard != dialog->tempSettings.keyboardLockEnabled || 
                        oldMouse != dialog->tempSettings.mouseLockEnabled) {
                        dialog->hasUnsavedChanges = true;
                    }
                    
                    // Update warnings when lock type changes
                    dialog->UpdateWarnings();
                    break;
                }
                
                case IDC_RADIO_PASSWORD:
                case IDC_RADIO_TIMER: {
                    int oldMethod = dialog->tempSettings.unlockMethod;
                    dialog->tempSettings.unlockMethod = LOWORD(wParam) - IDC_RADIO_PASSWORD;
                    
                    if (oldMethod != dialog->tempSettings.unlockMethod) {
                        dialog->hasUnsavedChanges = true;
                    }
                    
                    // Update warnings when unlock method changes
                    dialog->UpdateWarnings();
                    break;
                }
                
                case IDC_CHECK_WHITELIST: {
                    bool oldValue = dialog->tempSettings.whitelistEnabled;
                    dialog->tempSettings.whitelistEnabled = (IsDlgButtonChecked(hDlg, IDC_CHECK_WHITELIST) == BST_CHECKED);
                    
                    // Enable/disable whitelist configuration button
                    EnableWindow(GetDlgItem(hDlg, IDC_BTN_WHITELIST_CFG), dialog->tempSettings.whitelistEnabled);
                    
                    if (oldValue != dialog->tempSettings.whitelistEnabled) {
                        dialog->hasUnsavedChanges = true;
                    }
                    break;
                }
                
                case IDC_BTN_PASSWORD_CFG:
                    dialog->ShowPasswordConfig();
                    break;
                    
                case IDC_BTN_TIMER_CFG:
                    dialog->ShowTimerConfig();
                    break;
                    
                case IDC_BTN_WHITELIST_CFG:
                    dialog->ShowWhitelistConfig();
                    break;
                    
                case IDC_EDIT_HOTKEY_LOCK:
                    if (HIWORD(wParam) == EN_SETFOCUS) {
                        // User clicked textbox - start capture using modular system
                        HWND hEdit = GetDlgItem(hDlg, IDC_EDIT_HOTKEY_LOCK);
                        HWND hHint = GetDlgItem(hDlg, IDC_LABEL_HOTKEY_HINT);
                        g_hotkeyManager.StartCapture(hDlg, hEdit, hHint, dialog->tempSettings.lockHotkey);
                    }
                    break;
            }
            break;
        }
    }
    
    return FALSE;
}

INT_PTR CALLBACK SettingsDialog::ProductivityTabProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    SettingsDialog* dialog = nullptr;
    
    if (message == WM_INITDIALOG) {
        dialog = (SettingsDialog*)lParam;
        SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)dialog);
    } else {
        dialog = (SettingsDialog*)GetWindowLongPtr(hDlg, GWLP_USERDATA);
    }
    
    if (!dialog) return FALSE;
    
    switch (message) {
        case WM_INITDIALOG: {
            // Initialize productivity controls from current settings
            CheckDlgButton(hDlg, IDC_CHECK_USB_ALERT, dialog->tempSettings.usbAlertEnabled ? BST_CHECKED : BST_UNCHECKED);
            CheckDlgButton(hDlg, IDC_CHECK_QUICK_LAUNCH, dialog->tempSettings.quickLaunchEnabled ? BST_CHECKED : BST_UNCHECKED);
            CheckDlgButton(hDlg, IDC_CHECK_TIMER, dialog->tempSettings.workBreakTimerEnabled ? BST_CHECKED : BST_UNCHECKED);
            
            // Initialize boss key controls  
            CheckDlgButton(hDlg, IDC_CHECK_BOSS_KEY, dialog->tempSettings.bossKeyEnabled ? BST_CHECKED : BST_UNCHECKED);
            SetDlgItemTextA(hDlg, IDC_EDIT_HOTKEY_BOSS, dialog->tempSettings.bossKeyHotkey.c_str());
            EnableWindow(GetDlgItem(hDlg, IDC_EDIT_HOTKEY_BOSS), dialog->tempSettings.bossKeyEnabled);
            EnableWindow(GetDlgItem(hDlg, IDC_BTN_BOSS_KEY_TEST), dialog->tempSettings.bossKeyEnabled);
            
            return TRUE;
        }
        
        case WM_COMMAND: {
            switch (LOWORD(wParam)) {
                case IDC_CHECK_USB_ALERT: {
                    bool oldValue = dialog->tempSettings.usbAlertEnabled;
                    dialog->tempSettings.usbAlertEnabled = (IsDlgButtonChecked(hDlg, IDC_CHECK_USB_ALERT) == BST_CHECKED);
                    
                    if (oldValue != dialog->tempSettings.usbAlertEnabled) {
                        dialog->hasUnsavedChanges = true;
                    }
                    break;
                }
                
                case IDC_CHECK_QUICK_LAUNCH: {
                    bool oldValue = dialog->tempSettings.quickLaunchEnabled;
                    dialog->tempSettings.quickLaunchEnabled = (IsDlgButtonChecked(hDlg, IDC_CHECK_QUICK_LAUNCH) == BST_CHECKED);
                    
                    if (oldValue != dialog->tempSettings.quickLaunchEnabled) {
                        dialog->hasUnsavedChanges = true;
                    }
                    break;
                }
                
                case IDC_CHECK_TIMER: {
                    bool oldValue = dialog->tempSettings.workBreakTimerEnabled;
                    dialog->tempSettings.workBreakTimerEnabled = (IsDlgButtonChecked(hDlg, IDC_CHECK_TIMER) == BST_CHECKED);
                    
                    if (oldValue != dialog->tempSettings.workBreakTimerEnabled) {
                        dialog->hasUnsavedChanges = true;
                    }
                    break;
                }
                
                case IDC_BTN_TIMER_CONFIG:
                    MessageBoxA(hDlg, "Timer Configuration:\n\nWork Duration: 25 minutes\nShort Break: 5 minutes\nLong Break: 15 minutes\n\n(Advanced configuration coming in next update)", 
                               "Pomodoro Timer Settings", MB_OK | MB_ICONINFORMATION);
                    break;
                    
                case IDC_BTN_QUICK_LAUNCH_CONFIG:
                    MessageBoxA(hDlg, "Quick Launch Configuration:\n\nDefault hotkeys:\nCtrl+F1 - Notepad\nCtrl+F2 - Calculator\nCtrl+Alt+F3 - File Explorer\n\n(Custom app configuration coming in next update)", 
                               "Quick Launch Settings", MB_OK | MB_ICONINFORMATION);
                    break;
                    
                case IDC_BTN_START_WORK_SESSION: {
                    extern ProductivityManager g_productivityManager;
                    if (dialog->tempSettings.workBreakTimerEnabled) {
                        if (g_productivityManager.StartWorkSession()) {
                            MessageBoxA(hDlg, "Work session started! You'll be notified when it's time for a break.\n\nTimer: 25 minutes work, 5 minute breaks\nLong break every 4 sessions", 
                                       "Pomodoro Timer", MB_OK | MB_ICONINFORMATION);
                        } else {
                            MessageBoxA(hDlg, "Failed to start work session. Make sure the timer feature is enabled and applied.", 
                                       "Error", MB_OK | MB_ICONERROR);
                        }
                    } else {
                        MessageBoxA(hDlg, "Please enable the Work/Break Timer feature first, then click Apply.", 
                                   "Timer Not Enabled", MB_OK | MB_ICONWARNING);
                    }
                    break;
                }
            }
            break;
        }
        
        case WM_DESTROY: {
            // Clean up font resources
            HFONT hFont = (HFONT)GetProp(hDlg, TEXT("DialogFont"));
            if (hFont) {
                DeleteObject(hFont);
                RemoveProp(hDlg, TEXT("DialogFont"));
            }
            return TRUE;
        }
    }
    
    return FALSE;
}

INT_PTR CALLBACK SettingsDialog::PrivacyTabProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    SettingsDialog* dialog = nullptr;
    
    if (message == WM_INITDIALOG) {
        dialog = (SettingsDialog*)lParam;
        SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)dialog);
    } else {
        dialog = (SettingsDialog*)GetWindowLongPtr(hDlg, GWLP_USERDATA);
    }
    
    if (!dialog) return FALSE;
    
    switch (message) {
        case WM_INITDIALOG: {
            // Initialize privacy settings from current settings
            CheckDlgButton(hDlg, IDC_CHECK_START_WINDOWS, dialog->tempSettings.startWithWindows ? BST_CHECKED : BST_UNCHECKED);
            
            // Initialize boss key controls  
            CheckDlgButton(hDlg, IDC_CHECK_BOSS_KEY, dialog->tempSettings.bossKeyEnabled ? BST_CHECKED : BST_UNCHECKED);
            SetDlgItemTextA(hDlg, IDC_EDIT_HOTKEY_BOSS, dialog->tempSettings.bossKeyHotkey.c_str());
            EnableWindow(GetDlgItem(hDlg, IDC_EDIT_HOTKEY_BOSS), dialog->tempSettings.bossKeyEnabled);
            EnableWindow(GetDlgItem(hDlg, IDC_BTN_BOSS_KEY_TEST), dialog->tempSettings.bossKeyEnabled);
            
            return TRUE;
        }
        
        case WM_COMMAND: {
            switch (LOWORD(wParam)) {
                case IDC_CHECK_START_WINDOWS: {
                    bool oldValue = dialog->tempSettings.startWithWindows;
                    dialog->tempSettings.startWithWindows = (IsDlgButtonChecked(hDlg, IDC_CHECK_START_WINDOWS) == BST_CHECKED);
                    
                    if (oldValue != dialog->tempSettings.startWithWindows) {
                        dialog->hasUnsavedChanges = true;
                    }
                    break;
                }
                
                // Boss key controls
                case IDC_CHECK_BOSS_KEY: {
                    bool oldValue = dialog->tempSettings.bossKeyEnabled;
                    dialog->tempSettings.bossKeyEnabled = (IsDlgButtonChecked(hDlg, IDC_CHECK_BOSS_KEY) == BST_CHECKED);
                    
                    // Enable/disable related controls
                    EnableWindow(GetDlgItem(hDlg, IDC_EDIT_HOTKEY_BOSS), dialog->tempSettings.bossKeyEnabled);
                    EnableWindow(GetDlgItem(hDlg, IDC_BTN_BOSS_KEY_TEST), dialog->tempSettings.bossKeyEnabled);
                    
                    if (oldValue != dialog->tempSettings.bossKeyEnabled) {
                        dialog->hasUnsavedChanges = true;
                    }
                    break;
                }
                
                case IDC_EDIT_HOTKEY_BOSS:
                    if (HIWORD(wParam) == EN_SETFOCUS && dialog->tempSettings.bossKeyEnabled) {
                        // User clicked boss key textbox - start capture
                        HWND hEdit = GetDlgItem(hDlg, IDC_EDIT_HOTKEY_BOSS);
                        g_hotkeyManager.StartCapture(hDlg, hEdit, NULL, dialog->tempSettings.bossKeyHotkey);
                    }
                    break;
                
                case IDC_BTN_BOSS_KEY_TEST: {
                    extern PrivacyManager g_privacyManager;
                    if (g_privacyManager.IsBossKeyActive()) {
                        g_privacyManager.DeactivateBossKey();
                        MessageBoxA(hDlg, "Boss Key deactivated! All windows have been restored.", 
                                   "Boss Key Test", MB_OK | MB_ICONINFORMATION);
                    } else {
                        MessageBoxA(hDlg, "Testing Boss Key... All windows will be hidden for 3 seconds!", 
                                   "Boss Key Test", MB_OK | MB_ICONINFORMATION);
                        g_privacyManager.ActivateBossKey();
                        Sleep(3000);
                        g_privacyManager.DeactivateBossKey();
                    }
                    break;
                }
            }
            break;
        }
        
        case WM_DESTROY: {
            // Clean up font resources
            HFONT hFont = (HFONT)GetProp(hDlg, TEXT("DialogFont"));
            if (hFont) {
                DeleteObject(hFont);
                RemoveProp(hDlg, TEXT("DialogFont"));
            }
            return TRUE;
        }
    }
    
    return FALSE;
}

INT_PTR CALLBACK SettingsDialog::AppearanceTabProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    SettingsDialog* dialog = nullptr;
    
    if (message == WM_INITDIALOG) {
        dialog = (SettingsDialog*)lParam;
        SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)dialog);
    } else {
        dialog = (SettingsDialog*)GetWindowLongPtr(hDlg, GWLP_USERDATA);
    }
    
    if (!dialog) return FALSE;
    
    switch (message) {
        case WM_INITDIALOG: {
            // Initialize overlay manager with current settings
            g_overlayManager.SetStyle((OverlayStyle)dialog->tempSettings.overlayStyle);
            g_overlayManager.InitializeRadioButtons(hDlg, IDC_RADIO_BLUR);
            
            // Set description
            SetDlgItemTextA(hDlg, IDC_LABEL_OVERLAY_DESC, 
                          "Choose the overlay style that appears when input is locked:");
            
            // Initialize notification style controls
            SetDlgItemTextA(hDlg, IDC_LABEL_NOTIFY_DESC,
                          "Choose notification style for system alerts:");
            
            // Set notification style radio buttons
            switch (dialog->tempSettings.notificationStyle) {
                case 0: // Custom
                    CheckRadioButton(hDlg, IDC_RADIO_NOTIFY_CUSTOM, IDC_RADIO_NOTIFY_NONE, IDC_RADIO_NOTIFY_CUSTOM);
                    break;
                case 1: // Windows
                    CheckRadioButton(hDlg, IDC_RADIO_NOTIFY_CUSTOM, IDC_RADIO_NOTIFY_NONE, IDC_RADIO_NOTIFY_WINDOWS);
                    break;
                case 2: // None
                    CheckRadioButton(hDlg, IDC_RADIO_NOTIFY_CUSTOM, IDC_RADIO_NOTIFY_NONE, IDC_RADIO_NOTIFY_NONE);
                    break;
            }
            
            return TRUE;
        }
        
        case WM_COMMAND: {
            switch (LOWORD(wParam)) {
                case IDC_RADIO_BLUR:
                case IDC_RADIO_DIM:
                case IDC_RADIO_BLACK:
                case IDC_RADIO_NONE: {
                    // Use modular overlay manager
                    int oldStyle = dialog->tempSettings.overlayStyle;
                    g_overlayManager.HandleRadioButtonClick(hDlg, LOWORD(wParam), IDC_RADIO_BLUR);
                    dialog->tempSettings.overlayStyle = g_overlayManager.GetStyle();
                    
                    if (oldStyle != dialog->tempSettings.overlayStyle) {
                        dialog->hasUnsavedChanges = true;
                    }
                    break;
                }
                
                case IDC_RADIO_NOTIFY_CUSTOM:
                case IDC_RADIO_NOTIFY_WINDOWS:
                case IDC_RADIO_NOTIFY_NONE: {
                    int oldNotifyStyle = dialog->tempSettings.notificationStyle;
                    
                    if (LOWORD(wParam) == IDC_RADIO_NOTIFY_CUSTOM) {
                        dialog->tempSettings.notificationStyle = 0;
                    } else if (LOWORD(wParam) == IDC_RADIO_NOTIFY_WINDOWS) {
                        dialog->tempSettings.notificationStyle = 1;
                    } else if (LOWORD(wParam) == IDC_RADIO_NOTIFY_NONE) {
                        dialog->tempSettings.notificationStyle = 2;
                    }
                    
                    // Update notification system style
                    if (g_customNotifications) {
                        g_customNotifications->SetStyle((NotificationStyle)dialog->tempSettings.notificationStyle);
                    }
                    
                    if (oldNotifyStyle != dialog->tempSettings.notificationStyle) {
                        dialog->hasUnsavedChanges = true;
                    }
                    break;
                }
            }
            break;
        }
    }
    
    return FALSE;
}

void SettingsDialog::LoadSettings() {
    tempSettings = *settings;
    originalSettings = *settings; // Update baseline
    hasUnsavedChanges = false;
}

void SettingsDialog::SaveSettings() {
    // First, read all current values from UI into tempSettings
    ReadUIValues();
    
    // Use modular settings core for saving
    if (g_settingsCore.SaveSettings(tempSettings)) {
        *settings = tempSettings; // Update the original settings pointer
        originalSettings = tempSettings; // Update baseline
        hasUnsavedChanges = false;
    }
}

void SettingsDialog::ReadUIValues() {
    // Read values from Lock & Input tab
    if (hTabLockInput) {
        // Read checkboxes
        tempSettings.keyboardLockEnabled = IsDlgButtonChecked(hTabLockInput, IDC_CHECK_KEYBOARD) == BST_CHECKED;
        tempSettings.mouseLockEnabled = IsDlgButtonChecked(hTabLockInput, IDC_CHECK_MOUSE) == BST_CHECKED;
        
        // Read radio buttons for unlock method (only Password and Timer now)
        if (IsDlgButtonChecked(hTabLockInput, IDC_RADIO_PASSWORD) == BST_CHECKED) {
            tempSettings.unlockMethod = 0;
        } else if (IsDlgButtonChecked(hTabLockInput, IDC_RADIO_TIMER) == BST_CHECKED) {
            tempSettings.unlockMethod = 1;
        }
        
        // Read whitelist checkbox separately (it's now an addon feature)
        tempSettings.whitelistEnabled = IsDlgButtonChecked(hTabLockInput, IDC_CHECK_WHITELIST) == BST_CHECKED;
        
        // Read hotkey text
        char hotkeyBuffer[256];
        GetDlgItemTextA(hTabLockInput, IDC_EDIT_HOTKEY_LOCK, hotkeyBuffer, sizeof(hotkeyBuffer));
        tempSettings.lockHotkey = std::string(hotkeyBuffer);
        
        // Convert hotkey string to modifiers and virtual key using the parsing function
        extern bool ParseHotkeyString(const std::string& hotkeyStr, UINT& modifiers, UINT& virtualKey);
        ParseHotkeyString(tempSettings.lockHotkey, 
                         (UINT&)tempSettings.hotkeyModifiers, 
                         (UINT&)tempSettings.hotkeyVirtualKey);
    }
    
    // Read values from Productivity tab
    if (hTabProductivity) {
        tempSettings.usbAlertEnabled = IsDlgButtonChecked(hTabProductivity, IDC_CHECK_USB_ALERT) == BST_CHECKED;
        tempSettings.quickLaunchEnabled = IsDlgButtonChecked(hTabProductivity, IDC_CHECK_QUICK_LAUNCH) == BST_CHECKED;
        tempSettings.workBreakTimerEnabled = IsDlgButtonChecked(hTabProductivity, IDC_CHECK_TIMER) == BST_CHECKED;
    }
    
    // Read values from Privacy tab
    if (hTabPrivacy) {
        tempSettings.startWithWindows = IsDlgButtonChecked(hTabPrivacy, IDC_CHECK_START_WINDOWS) == BST_CHECKED;
        tempSettings.bossKeyEnabled = IsDlgButtonChecked(hTabPrivacy, IDC_CHECK_BOSS_KEY) == BST_CHECKED;
        
        // Read boss key hotkey
        char bossKeyBuffer[256];
        GetDlgItemTextA(hTabPrivacy, IDC_EDIT_HOTKEY_BOSS, bossKeyBuffer, sizeof(bossKeyBuffer));
        tempSettings.bossKeyHotkey = std::string(bossKeyBuffer);
    }
    
    // Read values from Appearance tab
    if (hTabAppearance) {
        tempSettings.overlayStyle = g_overlayManager.GetStyle();
        
        // Read notification style
        if (IsDlgButtonChecked(hTabAppearance, IDC_RADIO_NOTIFY_CUSTOM) == BST_CHECKED) {
            tempSettings.notificationStyle = 0;
        } else if (IsDlgButtonChecked(hTabAppearance, IDC_RADIO_NOTIFY_WINDOWS) == BST_CHECKED) {
            tempSettings.notificationStyle = 1;
        } else if (IsDlgButtonChecked(hTabAppearance, IDC_RADIO_NOTIFY_NONE) == BST_CHECKED) {
            tempSettings.notificationStyle = 2;
        }
    }
}

void SettingsDialog::ApplySettings() {
    // Use modular settings core for applying settings
    if (g_settingsCore.ApplySettings(tempSettings, g_mainWindow)) {
        *settings = tempSettings; // Update the original settings pointer
        
        // Refresh input hooks based on new keyboard/mouse lock settings
        RefreshHooks();
        
        // Re-register hotkeys with new settings
        if (g_mainWindow) {
            RegisterHotkeyFromSettings(g_mainWindow);
        }
        
        hasUnsavedChanges = false;
    }
}

bool SettingsDialog::HasPendingChanges() {
    // Use modular settings core for change detection
    return g_settingsCore.HasChanges(tempSettings, originalSettings);
}

void SettingsDialog::ShowPasswordConfig() {
    MessageBoxA(hMainDialog, "Password configuration dialog coming soon!", "Password Settings", MB_OK | MB_ICONINFORMATION);
}

void SettingsDialog::ShowTimerConfig() {
    MessageBoxA(hMainDialog, "Timer configuration dialog coming soon!", "Timer Settings", MB_OK | MB_ICONINFORMATION);
}

void SettingsDialog::ShowWhitelistConfig() {
    MessageBoxA(hMainDialog, "Whitelist configuration dialog coming soon!", "Whitelist Settings", MB_OK | MB_ICONINFORMATION);
}

void SettingsDialog::UpdateWarnings() {
    if (!hTabLockInput) return;
    
    // Get current settings from UI
    bool keyboardEnabled = IsDlgButtonChecked(hTabLockInput, IDC_CHECK_KEYBOARD) == BST_CHECKED;
    bool mouseEnabled = IsDlgButtonChecked(hTabLockInput, IDC_CHECK_MOUSE) == BST_CHECKED;
    bool passwordSelected = IsDlgButtonChecked(hTabLockInput, IDC_RADIO_PASSWORD) == BST_CHECKED;
    
    // Get hotkey text and check if it's a single key
    char hotkeyBuffer[256];
    GetDlgItemTextA(hTabLockInput, IDC_EDIT_HOTKEY_LOCK, hotkeyBuffer, sizeof(hotkeyBuffer));
    std::string currentHotkey = std::string(hotkeyBuffer);
    bool isSingleKey = g_hotkeyManager.IsSingleKey(currentHotkey);
    
    // Warning 1: Password won't work if keyboard is unlocked
    HWND hWarning1 = GetDlgItem(hTabLockInput, IDC_WARNING_KEYBOARD_UNLOCK);
    if (hWarning1 && !keyboardEnabled && passwordSelected) {
        SetWindowTextA(hWarning1, "!!WARNING!!: Password unlock will not work with keyboard unlocked.");
        ShowWindow(hWarning1, SW_SHOW);
    } else if (hWarning1) {
        ShowWindow(hWarning1, SW_HIDE);
    }
    
    // Warning 2: Locking mechanism disabled
    HWND hWarning2 = GetDlgItem(hTabLockInput, IDC_WARNING_LOCKING_DISABLED);
    if (hWarning2 && !keyboardEnabled && !mouseEnabled) {
        SetWindowTextA(hWarning2, "!!WARNING!!: Locking mechanism will be disabled.");
        ShowWindow(hWarning2, SW_SHOW);
    } else if (hWarning2) {
        ShowWindow(hWarning2, SW_HIDE);
    }
    
    // Warning 3: Single key hotkey
    HWND hWarning3 = GetDlgItem(hTabLockInput, IDC_WARNING_SINGLE_KEY);
    if (hWarning3 && isSingleKey && !currentHotkey.empty()) {
        SetWindowTextA(hWarning3, "!!WARNING!!: Single letter hotkeys are not recommended for security.");
        ShowWindow(hWarning3, SW_SHOW);
    } else if (hWarning3) {
        ShowWindow(hWarning3, SW_HIDE);
    }
}

void SettingsDialog::CreateWarningControls(HWND hDlg) {
    // Get dialog font for consistent styling
    HFONT hDialogFont = (HFONT)SendMessage(hDlg, WM_GETFONT, 0, 0);
    
    // Create Warning 1: Password won't work with keyboard unlocked
    // Position: Below the Input Types section (where locking disabled was)
    HWND hWarning1 = CreateWindowA("STATIC", "",
        WS_CHILD | SS_LEFT,
        20, 280, 360, 40,  // x, y, width, height (increased height for 2 lines)
        hDlg, (HMENU)IDC_WARNING_KEYBOARD_UNLOCK, GetModuleHandle(NULL), NULL);
    if (hWarning1) {
        SendMessage(hWarning1, WM_SETFONT, (WPARAM)hDialogFont, TRUE);
    }
    
    // Create Warning 2: Locking mechanism disabled
    // Position: Below the Input Types section
    HWND hWarning2 = CreateWindowA("STATIC", "",
        WS_CHILD | SS_LEFT,
        20, 320, 360, 30,  // x, y, width, height
        hDlg, (HMENU)IDC_WARNING_LOCKING_DISABLED, GetModuleHandle(NULL), NULL);
    if (hWarning2) {
        SendMessage(hWarning2, WM_SETFONT, (WPARAM)hDialogFont, TRUE);
    }
    
    // Create Warning 3: Single key hotkey warning
    // Position: Below the hotkey input
    HWND hWarning3 = CreateWindowA("STATIC", "",
        WS_CHILD | SS_LEFT,
        20, 360, 360, 30,  // x, y, width, height
        hDlg, (HMENU)IDC_WARNING_SINGLE_KEY, GetModuleHandle(NULL), NULL);
    if (hWarning3) {
        SendMessage(hWarning3, WM_SETFONT, (WPARAM)hDialogFont, TRUE);
    }
}

// Global settings functions
void InitializeSettings() {
    LoadSettingsFromFile();
}

void LoadSettingsFromFile() {
    // Load settings from configuration file
    // For now, use defaults
    g_appSettings = AppSettings{};
}

void SaveSettingsToFile() {
    // Save settings to configuration file
    // Implementation will depend on your preferred format (INI, JSON, etc.)
}

void ShowSettingsDialog(HWND parent) {
    SettingsDialog dialog(&g_appSettings);
    dialog.ShowDialog(parent);
}

std::string HotkeyToString(UINT modifiers, UINT virtualKey) {
    std::string result;
    
    if (modifiers & MOD_CONTROL) result += "Ctrl+";
    if (modifiers & MOD_SHIFT) result += "Shift+";
    if (modifiers & MOD_ALT) result += "Alt+";
    if (modifiers & MOD_WIN) result += "Win+";
    
    // Add the key name (simplified)
    result += (char)virtualKey;
    
    return result;
}
