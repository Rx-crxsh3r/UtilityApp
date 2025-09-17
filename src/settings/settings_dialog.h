// src/settings/settings_dialog.h
// Simplified settings dialog using modular components

#pragma once
#include <windows.h>
#include "settings_core.h"
#include "hotkey_manager.h"
#include "overlay_manager.h"

class SettingsDialog {
private:
    HWND hMainDialog;
    HWND hTabControl;
    HWND hCurrentTab;
    int currentTabIndex;
    
    // Tab dialog handles
    HWND hTabLockInput;
    HWND hTabProductivity;
    HWND hTabPrivacy;
    HWND hTabAppearance;
    
    // Temporary settings for the session
    CoreSettings sessionSettings;

public:
    SettingsDialog();
    ~SettingsDialog();
    
    // Main dialog functions
    bool ShowDialog(HWND parent);
    static INT_PTR CALLBACK DialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    
    // Tab management
    void SwitchTab(int tabIndex);
    void CreateTabDialogs();
    void ShowTabDialog(HWND hTab);
    void HideCurrentTab();
    
    // Tab dialog procedures
    static INT_PTR CALLBACK LockInputTabProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK ProductivityTabProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK PrivacyTabProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK AppearanceTabProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    
    // Settings management
    void LoadSettings();
    void SaveSettings();
    void ApplySettings();
    bool HasPendingChanges();
    
    // UI helpers
    void UpdateWarningDisplay();
    void HandleTabSwitch(int newTab);
};

// Global function
void ShowSettingsDialog(HWND parent);
