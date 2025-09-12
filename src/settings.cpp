// src/settings.cpp
// Settings system implementation - using modular components

#include "settings.h"
#include "resource.h"
#include "input_blocker.h"
#include "features/privacy/privacy_manager.h"
#include "features/productivity/productivity_manager.h"
#include "features/lock_input/lock_input_tab.h"
#include "features/appearance/appearance_tab.h"
#include "features/data_management/data_tab.h"
#include "ui/productivity_tab.h"
#include "ui/privacy_tab.h"
#include "custom_notifications.h"
#include "notifications.h"
#include <commctrl.h>

// Global settings instances
AppSettings g_appSettings;        // Current runtime settings (what the app is using)
AppSettings g_persistentSettings; // Last saved settings (from registry/file)  
bool g_settingsLoaded = false;

// Global manager instances
extern ProductivityManager g_productivityManager;
extern PrivacyManager g_privacyManager;

// Static pointer for dialog procedures
static SettingsDialog* g_currentDialog = nullptr;

SettingsDialog::SettingsDialog(AppSettings* appSettings) 
    : hMainDialog(nullptr), hTabControl(nullptr), hCurrentTab(nullptr),
      currentTabIndex(0), settings(appSettings), hasUnsavedChanges(false),
      hTabLockInput(nullptr), hTabProductivity(nullptr),
      hTabPrivacy(nullptr), hTabAppearance(nullptr), hTabData(nullptr),
      lockInputTab(nullptr), productivityTab(nullptr), privacyTab(nullptr), 
      appearanceTab(nullptr), dataTab(nullptr) {
    
    // Initialize with current runtime settings
    tempSettings = g_appSettings;
    
    // Update the passed pointer to current runtime settings
    if (appSettings) {
        *appSettings = g_appSettings;
    }

    // Create the tab objects
    lockInputTab = new LockInputTab(this, &tempSettings, &hasUnsavedChanges);
    productivityTab = new ProductivityTab(this, &tempSettings, &hasUnsavedChanges);
    privacyTab = new PrivacyTab(this, &tempSettings, &hasUnsavedChanges);
    appearanceTab = new AppearanceTab(this, &tempSettings, &hasUnsavedChanges);
    dataTab = new DataTab(this, &tempSettings, &hasUnsavedChanges);
}

SettingsDialog::~SettingsDialog() {
    // Clean up tab windows
    if (hTabLockInput) DestroyWindow(hTabLockInput);
    if (hTabProductivity) DestroyWindow(hTabProductivity);
    if (hTabPrivacy) DestroyWindow(hTabPrivacy);
    if (hTabAppearance) DestroyWindow(hTabAppearance);
    if (hTabData) DestroyWindow(hTabData);

    // Clean up tab objects
    if (lockInputTab) delete lockInputTab;
    if (productivityTab) delete productivityTab;
    if (privacyTab) delete privacyTab;
    if (appearanceTab) delete appearanceTab;
    if (dataTab) delete dataTab;
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
            
            tie.pszText = (LPSTR)"Data";
            TabCtrl_InsertItem(dialog->hTabControl, 4, &tie);
            
            // Load settings BEFORE creating tabs to ensure UI shows correct values
            dialog->LoadSettings();
            
            // Create tab dialogs
            dialog->CreateTabDialogs();
            
            // Refresh all tabs to ensure UI reflects loaded settings
            dialog->RefreshAllTabs();
            
            // Set the tab control to show the first tab and switch to it
            TabCtrl_SetCurSel(dialog->hTabControl, 0);
            dialog->SwitchTab(0); // Show first tab (Lock & Input)
            
            return TRUE;
        }
        
        case WM_NOTIFY: {
            LPNMHDR pnmh = (LPNMHDR)lParam;
            if (pnmh->idFrom == IDC_TAB_CONTROL && pnmh->code == TCN_SELCHANGE) {
                int newTab = TabCtrl_GetCurSel(dialog->hTabControl);
                if (newTab != dialog->currentTabIndex) {
                    // Check if there are unapplied changes (UI vs runtime)
                    // Note: tempSettings is maintained by tab controls, no need to call ReadUIValues()
                    if (g_settingsCore.HasChanges(dialog->tempSettings, g_appSettings)) {
                        int result = MessageBoxA(dialog->hMainDialog,
                                               "You have unapplied changes. Do you want to apply them?",
                                               "Unapplied Changes", 
                                               MB_YESNOCANCEL | MB_ICONQUESTION);
                        switch (result) {
                            case IDYES:
                                dialog->ApplySettings(); // Apply to runtime (like Apply button)
                                break;
                            case IDNO:
                                // Revert to current runtime settings
                                dialog->tempSettings = g_appSettings;
                                dialog->hasUnsavedChanges = g_settingsCore.HasChanges(g_appSettings, g_persistentSettings);
                                dialog->RefreshCurrentTabControls(); // Refresh UI to show runtime values
                                dialog->UpdateButtonStates(); // Update button states after revert
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
                    // OK = Save changes permanently and close
                    dialog->SaveSettings(); // This handles both save and apply
                    EndDialog(hDlg, IDOK);
                    return TRUE;
                    
                case IDC_BTN_CANCEL:
                    // Cancel = Discard changes and close (with confirmation if pending)
                    if (dialog->HasPendingChanges()) {
                        int result = MessageBoxA(dialog->hMainDialog,
                                                "You have unsaved changes. Are you sure you want to discard them?",
                                                "Discard Changes?", 
                                                MB_YESNO | MB_ICONQUESTION);
                        if (result == IDNO) {
                            return TRUE; // Don't close
                        }
                        // Note: We do NOT change g_appSettings here
                        // Runtime settings remain as they are, only dialog settings are discarded
                    }
                    EndDialog(hDlg, IDCANCEL);
                    return TRUE;
                    
                case IDC_BTN_APPLY:
                    // Apply = Apply changes temporarily (do not save to registry)
                    dialog->ApplySettings();
                    dialog->UpdateButtonStates();
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
                // Note: We do NOT change g_appSettings here
                // Runtime settings remain as they are, only dialog settings are discarded
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
    
    // Store the tab rectangle for lazy creation
    tabRect = rcTab;
    
    // Only create the first tab (Lock & Input) initially for better startup performance
    CreateSingleTab(TAB_LOCK_INPUT);
}

void SettingsDialog::CreateSingleTab(int tabIndex) {
    HWND* tabHandle = nullptr;
    int dialogResource = 0;
    DLGPROC dialogProc = nullptr;
    LPARAM dialogParam = 0;
    
    switch(tabIndex) {
        case TAB_LOCK_INPUT:
            if (hTabLockInput) return; // Already created
            tabHandle = &hTabLockInput;
            dialogResource = IDD_TAB_LOCK_INPUT;
            dialogProc = LockInputTab::DialogProc;
            dialogParam = (LPARAM)lockInputTab;
            break;
        case TAB_PRODUCTIVITY:
            if (hTabProductivity) return;
            tabHandle = &hTabProductivity;
            dialogResource = IDD_TAB_PRODUCTIVITY;
            dialogProc = ProductivityTab::DialogProc;
            dialogParam = (LPARAM)productivityTab;
            break;
        case TAB_PRIVACY:
            if (hTabPrivacy) return;
            tabHandle = &hTabPrivacy;
            dialogResource = IDD_TAB_PRIVACY;
            dialogProc = PrivacyTab::DialogProc;
            dialogParam = (LPARAM)privacyTab;
            break;
        case TAB_APPEARANCE:
            if (hTabAppearance) return;
            tabHandle = &hTabAppearance;
            dialogResource = IDD_TAB_APPEARANCE;
            dialogProc = AppearanceTab::DialogProc;
            dialogParam = (LPARAM)appearanceTab;
            break;
        case TAB_DATA:
            if (hTabData) return;
            tabHandle = &hTabData;
            dialogResource = IDD_TAB_DATA;
            dialogProc = DataTab::DialogProc;
            dialogParam = (LPARAM)dataTab;
            break;
        default:
            return;
    }
    
    // Create the tab dialog
    *tabHandle = CreateDialogParam(GetModuleHandle(NULL),
                                  MAKEINTRESOURCE(dialogResource),
                                  hMainDialog, dialogProc, dialogParam);
    
    // Position the tab dialog
    if (*tabHandle) {
        SetWindowPos(*tabHandle, NULL, tabRect.left, tabRect.top,
                    tabRect.right - tabRect.left, tabRect.bottom - tabRect.top,
                    SWP_NOZORDER);
        ShowWindow(*tabHandle, SW_HIDE); // Start hidden
    }
}

void SettingsDialog::SwitchTab(int tabIndex) {
    // End any active hotkey capture when switching tabs
    extern HotkeyManager g_hotkeyManager;
    g_hotkeyManager.EndCapture(false); // Don't save the hotkey
    
    HideCurrentTab();
    currentTabIndex = tabIndex;
    
    // Ensure tab control selection matches
    if (hTabControl) {
        TabCtrl_SetCurSel(hTabControl, tabIndex);
    }
    
    // Create tab if it doesn't exist (lazy loading)
    CreateSingleTab(tabIndex);
    
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
        case TAB_DATA:
            ShowTabDialog(hTabData);
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
        case TAB_DATA:
            if (dataTab) {
                dataTab->UpdateUI(hTabData);
            }
            break;
    }
}

void SettingsDialog::LoadSettings() {
    // Load settings from persistent storage first
    if (g_settingsCore.LoadSettings(tempSettings)) {
        // Successfully loaded saved settings - sync all layers
        g_settingsCore.UpdateAllLayers(tempSettings);
    } else {
        // No saved settings found - use defaults for all layers
        g_settingsCore.ResetToDefaults(tempSettings);
        g_settingsCore.UpdateAllLayers(tempSettings);
    }
    
    hasUnsavedChanges = false;
    RefreshUI(); // Centralized UI refresh
}

void SettingsDialog::RefreshAllTabs() {
    // Refresh all tab UI controls to reflect current tempSettings
    if (lockInputTab) {
        lockInputTab->RefreshControls();
    }
    if (productivityTab) {
        productivityTab->RefreshControls();
    }
    if (privacyTab) {
        privacyTab->RefreshControls();
    }
    if (appearanceTab) {
        appearanceTab->RefreshControls();
    }
    if (dataTab) {
        dataTab->RefreshControls();
    }
}

void SettingsDialog::RefreshUI() {
    // Centralized UI refresh - combines both operations for efficiency
    RefreshAllTabs();
    UpdateButtonStates();
}

void SettingsDialog::UpdateButtonStates() {
    // Update the state of OK/Apply buttons based on whether there are changes
    if (hMainDialog) {
        // Apply button: enabled when UI differs from current runtime settings
        // Note: Don't call ReadUIValues() here as tab classes maintain tempSettings directly
        bool hasRuntimeChanges = g_settingsCore.HasChanges(tempSettings, g_appSettings);
        
        EnableWindow(GetDlgItem(hMainDialog, IDC_BTN_APPLY), hasRuntimeChanges);
        
        // OK button should always be enabled
        EnableWindow(GetDlgItem(hMainDialog, IDC_BTN_OK), TRUE);
        
        // Update hasUnsavedChanges for other operations (comparing against persistent)
        hasUnsavedChanges = g_settingsCore.HasChanges(tempSettings, g_persistentSettings);
    }
}

void SettingsDialog::SaveSettings() {
    // tempSettings is maintained by tab classes - no need to read UI values
    
    // Check if current settings are default settings
    bool isDefaultSettings = (tempSettings == g_settingsCore.GetDefaultSettings());
    
    bool success = false;
    if (isDefaultSettings) {
        // If saving default settings, clear persistent storage instead
        // This ensures next app startup loads defaults from constructor
        success = g_settingsCore.ClearPersistentStorage();
        if (success) {
            // Update persistent settings to reflect that storage is cleared
            g_settingsCore.ResetToDefaults(g_persistentSettings);
            g_settingsCore.UpdateAllLayers(g_persistentSettings);
        }
    } else {
        // Save current temp settings normally
        success = g_settingsCore.SaveSettings(tempSettings);
        if (success) {
            // Update all layers with the saved settings
            g_settingsCore.UpdateAllLayers(tempSettings);
        }
    }
    
    if (success) {
        // Update the original settings pointer if provided
        if (settings) {
            *settings = tempSettings;
        }
        
        hasUnsavedChanges = false;
        
        // Apply the saved settings to the runtime system
        RefreshHooks();
        if (g_mainWindow) {
            RegisterHotkeyFromSettings(g_mainWindow);
        }
        
        // Show save success notification
        ShowNotification(g_mainWindow, NOTIFY_SETTINGS_SAVED, "Settings saved successfully");
    } else {
        // Show save failure notification
        ShowNotification(g_mainWindow, NOTIFY_SETTINGS_ERROR, "Failed to save settings");
    }
}

void SettingsDialog::ReadUIValues() {
    // This method is now deprecated - tab classes maintain tempSettings directly
    // via HandleControlCommand() methods. This ensures immediate Apply button updates
    // and eliminates race conditions between UI updates and settings state.
    
    // Note: If any controls are added that don't go through tab classes,
    // add their reading logic here as a fallback.
}

void SettingsDialog::ApplySettings() {
    // Check if there are any changes compared to current runtime settings
    // Note: tempSettings is maintained by tab controls, don't call ReadUIValues()
    if (!g_settingsCore.HasChanges(tempSettings, g_appSettings)) {
        // No changes detected, show appropriate message
        ShowNotification(g_mainWindow, NOTIFY_SETTINGS_APPLIED, "No changes to apply");
        return;
    }
    
    // Apply settings temporarily to current session (does not save to registry)
    // Use optimized method that only applies changed categories
    if (g_settingsCore.ApplySettings(tempSettings, g_appSettings, g_mainWindow)) {
        // Update global runtime settings temporarily (NOT saved to registry)
        g_appSettings = tempSettings;
        
        // Refresh input hooks based on new keyboard/mouse lock settings
        RefreshHooks();
        
        // Re-register hotkeys with new settings
        if (g_mainWindow) {
            RegisterHotkeyFromSettings(g_mainWindow);
        }
        
        // Update button states after successful apply
        // tempSettings != g_persistentSettings means there are unsaved changes
        hasUnsavedChanges = g_settingsCore.HasChanges(tempSettings, g_persistentSettings);
        UpdateButtonStates();
    }
}

bool SettingsDialog::HasPendingChanges() {
    // Check if current UI state (tempSettings) differs from runtime (g_appSettings)
    // This is for tab switching - asking "apply changes to runtime?"
    return g_settingsCore.HasChanges(tempSettings, g_appSettings);
}

void SettingsDialog::ResetToDefaults() {
    // Use centralized default settings for all layers
    g_settingsCore.ResetToDefaults(tempSettings);
    g_settingsCore.UpdateAllLayers(tempSettings);
    
    // Refresh UI to show default values
    RefreshAllTabs();
    
    // Refresh runtime system with defaults
    RefreshHooks();
    
    if (g_mainWindow) {
        RegisterHotkeyFromSettings(g_mainWindow);
    }
    
    hasUnsavedChanges = false;
    RefreshUI(); // Centralized UI refresh
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
    // Check if persistent data exists and is complete
    if (g_settingsCore.IsPersistentDataComplete()) {
        // Load complete persistent data and use it for runtime
        if (g_settingsCore.LoadSettings(g_persistentSettings)) {
            g_settingsCore.UpdateAllLayers(g_persistentSettings);  // runtime = persistent
        } else {
            // Fallback to defaults if loading fails
            g_settingsCore.ResetToDefaults(g_appSettings);
            g_settingsCore.UpdateAllLayers(g_appSettings);
        }
    } else {
        // No complete persistent data - use defaults for both layers
        g_settingsCore.ResetToDefaults(g_appSettings);
        g_settingsCore.UpdateAllLayers(g_appSettings);
    }
    
    g_settingsLoaded = true;
}

void SaveSettingsToFile() {
    // Save settings to configuration file
    // Implementation will depend on your preferred format (INI, JSON, etc.)
}

void ShowSettingsDialog(HWND parent) {
    // Prevent multiple settings dialogs from opening simultaneously
    if (g_currentDialog != nullptr) {
        // If a dialog is already open, bring it to the foreground
        HWND hDialog = g_currentDialog->GetMainDialogHandle();
        if (hDialog) {
            SetForegroundWindow(hDialog);
        }
        return;
    }
    
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
