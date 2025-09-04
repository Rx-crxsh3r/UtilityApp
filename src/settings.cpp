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
      hKeyboardHook(nullptr), lockInputTab(nullptr), productivityTab(nullptr), privacyTab(nullptr), appearanceTab(nullptr) {
    
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

    // Create the tab objects
    lockInputTab = new LockInputTab(this, &tempSettings, &hasUnsavedChanges);
    productivityTab = new ProductivityTab(this, &tempSettings, &hasUnsavedChanges);
    privacyTab = new PrivacyTab(this, &tempSettings, &hasUnsavedChanges);
    appearanceTab = new AppearanceTab(this, &tempSettings, &hasUnsavedChanges);
}

SettingsDialog::~SettingsDialog() {
    if (hKeyboardHook) {
        UnhookWindowsHookEx(hKeyboardHook);
    }
    if (hTabLockInput) DestroyWindow(hTabLockInput);
    if (hTabProductivity) DestroyWindow(hTabProductivity);
    if (hTabPrivacy) DestroyWindow(hTabPrivacy);
    if (hTabAppearance) DestroyWindow(hTabAppearance);

    // Clean up the lock input tab object
    if (lockInputTab) {
        delete lockInputTab;
        lockInputTab = nullptr;
    }

    // Clean up the productivity tab object
    if (productivityTab) {
        delete productivityTab;
        productivityTab = nullptr;
    }

    // Clean up the privacy tab object
    if (privacyTab) {
        delete privacyTab;
        privacyTab = nullptr;
    }

    // Clean up the appearance tab object
    if (appearanceTab) {
        delete appearanceTab;
        appearanceTab = nullptr;
    }
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
                                     hMainDialog, LockInputTab::DialogProc, (LPARAM)lockInputTab);
    
    // Position the tab dialog
    if (hTabLockInput) {
        SetWindowPos(hTabLockInput, NULL, rcTab.left, rcTab.top,
                    rcTab.right - rcTab.left, rcTab.bottom - rcTab.top,
                    SWP_NOZORDER);
    }
    
    // Create other tabs (for now, simple message dialogs)
    hTabProductivity = CreateDialogParam(GetModuleHandle(NULL),
                                        MAKEINTRESOURCE(IDD_TAB_PRODUCTIVITY),
                                        hMainDialog, ProductivityTab::DialogProc, (LPARAM)productivityTab);
    if (hTabProductivity) {
        SetWindowPos(hTabProductivity, NULL, rcTab.left, rcTab.top,
                    rcTab.right - rcTab.left, rcTab.bottom - rcTab.top,
                    SWP_NOZORDER);
    }
    
    hTabPrivacy = CreateDialogParam(GetModuleHandle(NULL),
                                   MAKEINTRESOURCE(IDD_TAB_PRIVACY),
                                   hMainDialog, PrivacyTab::DialogProc, (LPARAM)privacyTab);
    if (hTabPrivacy) {
        SetWindowPos(hTabPrivacy, NULL, rcTab.left, rcTab.top,
                    rcTab.right - rcTab.left, rcTab.bottom - rcTab.top,
                    SWP_NOZORDER);
    }
    
    hTabAppearance = CreateDialogParam(GetModuleHandle(NULL),
                                      MAKEINTRESOURCE(IDD_TAB_APPEARANCE),
                                      hMainDialog, AppearanceTab::DialogProc, (LPARAM)appearanceTab);
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
            if (lockInputTab) {
                lockInputTab->RefreshControls();
            }
            break;
        case TAB_PRODUCTIVITY:
            if (productivityTab) {
                productivityTab->RefreshControls();
            }
            break;
        case TAB_PRIVACY:
            if (privacyTab) {
                privacyTab->RefreshControls();
            }
            break;
        case TAB_APPEARANCE:
            if (appearanceTab) {
                appearanceTab->RefreshControls();
            }
            break;
    }
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
        } else if (IsDlgButtonChecked(hTabAppearance, IDC_RADIO_NOTIFY_WINDOWS_NOTIF) == BST_CHECKED) {
            tempSettings.notificationStyle = 2;
        } else if (IsDlgButtonChecked(hTabAppearance, IDC_RADIO_NOTIFY_NONE) == BST_CHECKED) {
            tempSettings.notificationStyle = 3;
        }
    }
}

void SettingsDialog::ApplySettings() {
    // Update global settings first so notifications respect new settings
    *settings = tempSettings;
    g_appSettings = tempSettings;
    
    // Use modular settings core for applying settings
    if (g_settingsCore.ApplySettings(tempSettings, g_mainWindow)) {
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
