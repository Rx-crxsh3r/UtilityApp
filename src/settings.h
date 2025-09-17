// src/settings.h
// Settings system header - now using modular components

#pragma once
#include <windows.h>
#include <string>

// Include modular components (settings_core.h contains AppSettings)
#include "settings/settings_core.h"
#include "features/lock_input/hotkey_manager.h" 
#include "features/appearance/overlay_manager.h"
#include "features/lock_input/lock_input_tab.h"
#include "ui/productivity_tab.h"
#include "ui/privacy_tab.h"
#include "features/appearance/appearance_tab.h"
#include "features/data_management/data_tab.h"

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
#define IDD_TAB_DATA            214

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
// Note: All control IDs are now defined in resource.h

// Appearance Tab Controls
#define IDC_RADIO_BLUR          240
#define IDC_RADIO_DIM           241
#define IDC_RADIO_BLACK         242
#define IDC_RADIO_NONE          243
#define IDC_LABEL_OVERLAY_DESC  244

// Button Controls
#define IDC_BTN_OK              260
#define IDC_BTN_CANCEL          261
#define IDC_BTN_APPLY           262

// Tab indices
enum SettingsTab {
    TAB_LOCK_INPUT = 0,
    TAB_PRODUCTIVITY = 1,
    TAB_PRIVACY = 2,
    TAB_APPEARANCE = 3,
    TAB_DATA = 4
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
    HWND hTabData;

    // Tab objects (OOP implementation)
    LockInputTab* lockInputTab;
    ProductivityTab* productivityTab;
    PrivacyTab* privacyTab;
    AppearanceTab* appearanceTab;
    DataTab* dataTab;
    
    // Lazy loading support
    RECT tabRect;

public:
    SettingsDialog(AppSettings* appSettings);
    ~SettingsDialog();
    
    // Main dialog functions
    bool ShowDialog(HWND parent);
    static INT_PTR CALLBACK DialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    
    // Dialog access
    HWND GetMainDialogHandle() const { return hMainDialog; }
    
    // Tab management
    void SwitchTab(int tabIndex);
    void CreateTabDialogs();
    void CreateSingleTab(int tabIndex);
    void ShowTabDialog(HWND hTab);
    void HideCurrentTab();
    void RefreshCurrentTabControls();
    void RefreshAllTabs();
    void UpdateButtonStates();
    void RefreshUI(); // Centralized UI refresh
    
    // Tab dialog procedures
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

// Global settings instances
extern AppSettings g_appSettings;          // Current runtime settings (what the app is using)
extern AppSettings g_persistentSettings;   // Last saved settings (from registry/file)
extern AppSettings g_tempSettings;         // Current session temporary settings (UI state)
extern bool g_settingsLoaded;

// Global main window handle (for hotkey registration)
extern HWND g_mainWindow;

// Settings utility functions
void InitializeSettings();
void LoadSettingsFromFile();
void SaveSettingsToFile();
void ShowSettingsDialog(HWND parent);
void RegisterHotkeyFromSettings(HWND hwnd);
std::string HotkeyToString(UINT modifiers, UINT virtualKey);
