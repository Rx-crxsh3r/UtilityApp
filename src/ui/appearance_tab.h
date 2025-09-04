// src/ui/appearance_tab.h
// Appearance tab header - OOP implementation

#pragma once

#include <windows.h>
#include <string>
#include "../settings/settings_core.h"

// Forward declarations
class SettingsDialog;

class AppearanceTab {
private:
    SettingsDialog* parentDialog;
    AppSettings* tempSettings;
    bool* hasUnsavedChanges;

    // Control handles
    HWND hTab;

public:
    // Constructor/Destructor
    AppearanceTab(SettingsDialog* parent, AppSettings* settings, bool* unsavedChanges);
    ~AppearanceTab();

    // Dialog procedure
    static INT_PTR CALLBACK DialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

    // Message handling
    INT_PTR HandleMessage(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

    // Control initialization and updates
    void InitializeControls(HWND hDlg);
    void RefreshControls();

    // Event handlers
    void OnOverlayStyleChanged(HWND hDlg, WPARAM wParam);
    void OnNotificationStyleChanged(HWND hDlg, WPARAM wParam);

    // Utility methods
    void UpdateNotificationStyleRadios(HWND hDlg);
};
