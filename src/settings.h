// src/settings.h
// Settings system header - now using modular components

#pragma once
#include <windows.h>
#include <string>

// Include modular components (settings_core.h contains AppSettings)
#include "settings/settings_core.h"
#include "settings/hotkey_manager.h" 
#include "settings/overlay_manager.h"

// Forward declarations
class PasswordManager;
class TimerManager;

// Settings Dialog Resource IDs
#define IDD_SETTINGS_DIALOG     200
#define IDC_TAB_CONTROL         201

// Tab Resource IDs  
#define IDD_TAB_LOCK_INPUT      210
#define IDD_TAB_PRODUCTIVITY    211
#define IDD_TAB_PRIVACY         212
#define IDD_TAB_APPEARANCE      213

// Lock & Input Tab Controls
#define IDC_CHECK_KEYBOARD      220
#define IDC_CHECK_MOUSE         221
#define IDC_RADIO_PASSWORD      222
#define IDC_RADIO_TIMER         223
#define IDC_CHECK_WHITELIST     224
#define IDC_EDIT_HOTKEY_LOCK    225
#define IDC_BTN_PASSWORD_CFG    226
// Extended Lock & Input Tab Controls (that are not in resource.h)
#define IDC_BTN_PASSWORD_CFG    226
#define IDC_BTN_TIMER_CFG       227
#define IDC_BTN_WHITELIST_CFG   228
#define IDC_BTN_SAVE_HOTKEY     229
#define IDC_BTN_CANCEL_HOTKEY   230
#define IDC_LABEL_HOTKEY_HINT   231
#define IDC_LABEL_HOTKEY_WARNING 232
// IDC_CHECK_UNLOCK_HOTKEY is now defined in resource.h as 307 (Privacy tab)
// IDC_EDIT_UNLOCK_HOTKEY is now defined in resource.h as 308 (Privacy tab)
#define IDC_LABEL_UNLOCK_HOTKEY 235
// IDC_BTN_TEST_UNLOCK is now defined in resource.h as 309 (Privacy tab)

// Note: Productivity and Privacy tab control IDs are defined in resource.h

// Appearance Tab Controls
#define IDC_RADIO_BLUR          240
#define IDC_RADIO_DIM           241
#define IDC_RADIO_BLACK         242
#define IDC_RADIO_NONE          243
#define IDC_LABEL_OVERLAY_DESC  244

// Placeholder Controls for Coming Soon tabs
#define IDC_LABEL_COMING_SOON   250

// Button Controls
#define IDC_BTN_OK              260
#define IDC_BTN_CANCEL          261
#define IDC_BTN_APPLY           262

// Tab indices
enum SettingsTab {
    TAB_LOCK_INPUT = 0,
    TAB_PRODUCTIVITY = 1,
    TAB_PRIVACY = 2,
    TAB_APPEARANCE = 3
};

// Settings Dialog Class
class SettingsDialog {
private:
    HWND hMainDialog;
    HWND hTabControl;
    HWND hCurrentTab;
    int currentTabIndex;
    AppSettings* settings;
    AppSettings tempSettings; // For unsaved changes
    AppSettings originalSettings; // For smart change detection
    bool hasUnsavedChanges;
    bool isEditingHotkey;
    
    // Hotkey capture state
    bool isCapturingHotkey;
    std::string currentHotkeyInput;
    std::string originalHotkey;
    bool ctrlPressed, shiftPressed, altPressed, winPressed;
    HHOOK hKeyboardHook; // For hotkey capture
    
    // Tab dialog handles
    HWND hTabLockInput;
    HWND hTabProductivity;
    HWND hTabPrivacy;
    HWND hTabAppearance;

public:
    SettingsDialog(AppSettings* appSettings);
    ~SettingsDialog();
    
    // Main dialog functions
    bool ShowDialog(HWND parent);
    static INT_PTR CALLBACK DialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    
    // Tab management
    void SwitchTab(int tabIndex);
    void CreateTabDialogs();
    void ShowTabDialog(HWND hTab);
    void HideCurrentTab();
    void RefreshCurrentTabControls();
    
    // Tab dialog procedures
    static INT_PTR CALLBACK LockInputTabProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK ProductivityTabProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK PrivacyTabProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK AppearanceTabProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    
    // Settings management
    void LoadSettings();
    void SaveSettings();
    void ReadUIValues(); // Read current values from UI controls
    void ApplySettings();
    void UpdateWarnings(); // Update warning messages based on current settings
    void CreateWarningControls(HWND hDlg); // Create warning labels dynamically
    bool PromptSaveChanges();
    bool HasPendingChanges(); // Smart change detection
    void ResetToDefaults();
    
    // Enhanced hotkey editing
    void StartHotkeyCapture();
    void EndHotkeyCapture(bool save);
    void UpdateHotkeyDisplay();
    void ValidateHotkey();
    static LRESULT CALLBACK HotkeyHookProc(int nCode, WPARAM wParam, LPARAM lParam);
    
    // Configuration dialogs
    void ShowPasswordConfig();
    void ShowTimerConfig();
    void ShowWhitelistConfig();
};

// Global settings instance
extern AppSettings g_appSettings;

// Global main window handle (for hotkey registration)
extern HWND g_mainWindow;

// Settings utility functions
void InitializeSettings();
void LoadSettingsFromFile();
void SaveSettingsToFile();
void ShowSettingsDialog(HWND parent);
void RegisterHotkeyFromSettings(HWND hwnd);
std::string HotkeyToString(UINT modifiers, UINT virtualKey);
