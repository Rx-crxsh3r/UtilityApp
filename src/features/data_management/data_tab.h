// src/features/data_management/data_tab.h
// Data tab header - Settings management and import/export functionality

#pragma once

#include <windows.h>
#include <string>
#include "../../settings/settings_core.h"

// Forward declarations
class SettingsDialog;

class DataTab {
private:
    SettingsDialog* parentDialog;
    AppSettings* tempSettings;
    bool* hasUnsavedChanges;

    // Control handles
    HWND hTab;

    // Helper methods
    std::string GetDownloadsPath();
    std::string GetSaveFilePath(bool useDownloads);
    std::string GetLoadFilePath();
    bool ConfirmReset();

public:
    // Constructor/Destructor
    DataTab(SettingsDialog* parent, AppSettings* settings, bool* unsavedChanges);
    ~DataTab();

    // Dialog procedure
    static INT_PTR CALLBACK DialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

    // Message handling
    void OnCommand(HWND hDlg, WPARAM wParam, LPARAM lParam);
    void OnInitDialog(HWND hDlg);

    // Button handlers
    void OnSaveSettings(HWND hDlg);
    void OnResetSettings(HWND hDlg);
    void OnLoadSettings(HWND hDlg);
    void OnExportSettings(HWND hDlg);

    // UI Updates
    void UpdateUI(HWND hDlg);
    void RefreshControls();
};
