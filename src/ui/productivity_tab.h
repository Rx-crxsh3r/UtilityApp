// src/ui/productivity_tab.h
// Productivity Tab - OOP implementation for better code organization

#pragma once
#include <windows.h>
#include <string>
#include "../settings/settings_core.h"

// Forward declarations
class SettingsDialog;

class ProductivityTab {
private:
    SettingsDialog* parentDialog;
    HWND hTabDialog;
    AppSettings* tempSettings;
    bool* hasUnsavedChanges;

    // Helper methods
    void InitializeControls();
    void HandleControlCommand(WPARAM wParam, LPARAM lParam);
    void ShowTimerConfig();
    void ShowQuickLaunchConfig();
    void StartWorkSession();

public:
    ProductivityTab(SettingsDialog* parent, AppSettings* settings, bool* unsavedFlag);
    ~ProductivityTab();

    // Main dialog procedure
    INT_PTR HandleMessage(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

    // Public interface
    void RefreshControls();
    void SetDialogHandle(HWND hDlg) { hTabDialog = hDlg; }
    HWND GetDialogHandle() const { return hTabDialog; }

    // Static callback for Windows dialog system
    static INT_PTR CALLBACK DialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
};
