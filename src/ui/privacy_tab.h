// src/ui/privacy_tab.h
// Privacy tab header - OOP implementation

#pragma once

#include <windows.h>
#include <string>
#include "../settings/settings_core.h"

// Forward declarations
class SettingsDialog;

class PrivacyTab {
private:
    SettingsDialog* parentDialog;
    AppSettings* tempSettings;
    bool* hasUnsavedChanges;

    // Control handles
    HWND hTab;

public:
    // Constructor/Destructor
    PrivacyTab(SettingsDialog* parent, AppSettings* settings, bool* unsavedChanges);
    ~PrivacyTab();

    // Dialog procedure
    static INT_PTR CALLBACK DialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

    // Message handling
    INT_PTR HandleMessage(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

    // Control initialization and updates
    void InitializeControls(HWND hDlg);
    void RefreshControls();

    // Event handlers
    void OnStartWithWindowsChanged(HWND hDlg);
    void OnBossKeyEnabledChanged(HWND hDlg);
    void OnBossKeyHotkeyFocus(HWND hDlg);
    void OnBossKeyTestClicked(HWND hDlg);

    // Utility methods
    void UpdateBossKeyControls(HWND hDlg);
};
