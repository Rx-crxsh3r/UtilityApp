// src/ui/lock_input_tab.h
// Lock Input Tab - OOP implementation for better code organization

#pragma once
#include <windows.h>
#include <string>
#include "../settings/settings_core.h"

// Forward declarations
class SettingsDialog;

class LockInputTab {
private:
    SettingsDialog* parentDialog;
    HWND hTabDialog;
    AppSettings* tempSettings;
    bool* hasUnsavedChanges;

    // Warning control handles
    HWND hWarningKeyboardUnlock;
    HWND hWarningLockingDisabled;
    HWND hWarningSingleKey;

    // Helper methods
    void InitializeControls();
    void UpdateWarnings();
    void CreateWarningControls();
    void HandleControlCommand(WPARAM wParam, LPARAM lParam);

public:
    LockInputTab(SettingsDialog* parent, AppSettings* settings, bool* unsavedFlag);
    ~LockInputTab();

    // Main dialog procedure
    INT_PTR HandleMessage(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

    // Public interface
    void RefreshControls();
    void SetDialogHandle(HWND hDlg) { hTabDialog = hDlg; }
    HWND GetDialogHandle() const { return hTabDialog; }

    // Configuration dialogs (delegated to parent)
    void ShowPasswordConfig();
    void ShowTimerConfig();
    void ShowWhitelistConfig();

    // Static callback for Windows dialog system
    static INT_PTR CALLBACK DialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
};
