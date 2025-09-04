// src/features/data_management/data_tab.cpp
// Data tab implementation - Settings management and import/export functionality

#include "data_tab.h"
#include "../../settings.h"
#include "../../notifications.h"
#include "../../settings/settings_core.h"
#include "../../resource.h"
#include <commdlg.h>
#include <shlobj.h>
#include <sstream>
#include <iomanip>
#include <ctime>

// External references
extern SettingsCore g_settingsCore;
extern HWND g_mainWindow;

DataTab::DataTab(SettingsDialog* parent, AppSettings* settings, bool* unsavedChanges) 
    : parentDialog(parent), tempSettings(settings), hasUnsavedChanges(unsavedChanges), hTab(NULL) {
}

DataTab::~DataTab() {
    if (hTab) {
        DestroyWindow(hTab);
        hTab = NULL;
    }
}

INT_PTR CALLBACK DataTab::DialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    DataTab* pThis = nullptr;
    
    if (message == WM_INITDIALOG) {
        pThis = reinterpret_cast<DataTab*>(lParam);
        SetWindowLongPtr(hDlg, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
        pThis->hTab = hDlg;
        pThis->OnInitDialog(hDlg);
        return TRUE;
    } else {
        pThis = reinterpret_cast<DataTab*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
    }
    
    if (pThis) {
        switch (message) {
            case WM_COMMAND:
                pThis->OnCommand(hDlg, wParam, lParam);
                return TRUE;
        }
    }
    
    return FALSE;
}

void DataTab::OnInitDialog(HWND hDlg) {
    UpdateUI(hDlg);
}

void DataTab::OnCommand(HWND hDlg, WPARAM wParam, LPARAM lParam) {
    switch (LOWORD(wParam)) {
        case IDC_BTN_SAVE_SETTINGS:
            OnSaveSettings(hDlg);
            break;
        case IDC_BTN_RESET_SETTINGS:
            OnResetSettings(hDlg);
            break;
        case IDC_BTN_LOAD_SETTINGS:
            OnLoadSettings(hDlg);
            break;
        case IDC_BTN_EXPORT_SETTINGS:
            OnExportSettings(hDlg);
            break;
    }
}

void DataTab::OnSaveSettings(HWND hDlg) {
    // Save current temp settings permanently
    if (g_settingsCore.SaveSettings(*tempSettings)) {
        *hasUnsavedChanges = false;
        ShowNotification(g_mainWindow, NOTIFY_SETTINGS_SAVED);
        
        // Update the parent dialog to reflect saved state
        if (parentDialog) {
            parentDialog->UpdateButtonStates();
        }
    } else {
        MessageBoxA(hDlg, "Failed to save settings. Please check permissions and try again.", 
                   "Save Error", MB_OK | MB_ICONERROR);
    }
}

void DataTab::OnResetSettings(HWND hDlg) {
    if (ConfirmReset()) {
        // Reset to default settings
        AppSettings defaultSettings;
        *tempSettings = defaultSettings;
        *hasUnsavedChanges = true;
        
        // Update all tabs to reflect the reset
        if (parentDialog) {
            parentDialog->RefreshAllTabs();
            parentDialog->UpdateButtonStates();
        }
        
        ShowNotification(g_mainWindow, NOTIFY_SETTINGS_RESET);
    }
}

void DataTab::OnLoadSettings(HWND hDlg) {
    std::string filepath = GetLoadFilePath();
    if (!filepath.empty()) {
        AppSettings loadedSettings;
        if (g_settingsCore.ImportFromFile(loadedSettings, filepath)) {
            *tempSettings = loadedSettings;
            *hasUnsavedChanges = true;
            
            // Update all tabs to reflect the loaded settings
            if (parentDialog) {
                parentDialog->RefreshAllTabs();
                parentDialog->UpdateButtonStates();
            }
            
            ShowNotification(g_mainWindow, NOTIFY_SETTINGS_LOADED);
        } else {
            MessageBoxA(hDlg, "Failed to load settings from the selected file. The file may be corrupted or in an invalid format.", 
                       "Load Error", MB_OK | MB_ICONERROR);
        }
    }
}

void DataTab::OnExportSettings(HWND hDlg) {
    // Ask user where to save
    int result = MessageBoxA(hDlg, 
        "Where would you like to save the settings file?\n\n"
        "Click 'Yes' to save to Downloads folder\n"
        "Click 'No' to choose a custom location\n"
        "Click 'Cancel' to abort",
        "Export Location", 
        MB_YESNOCANCEL | MB_ICONQUESTION);
        
    if (result == IDCANCEL) {
        return;
    }
    
    bool useDownloads = (result == IDYES);
    std::string filepath = GetSaveFilePath(useDownloads);
    
    if (!filepath.empty()) {
        if (g_settingsCore.ExportToFile(*tempSettings, filepath)) {
            std::string message = "Settings successfully exported to:\n" + filepath;
            MessageBoxA(hDlg, message.c_str(), "Export Success", MB_OK | MB_ICONINFORMATION);
        } else {
            MessageBoxA(hDlg, "Failed to export settings. Please check permissions and try again.", 
                       "Export Error", MB_OK | MB_ICONERROR);
        }
    }
}

void DataTab::UpdateUI(HWND hDlg) {
    // Enable/disable buttons based on current state
    // All buttons should always be enabled for this tab
    EnableWindow(GetDlgItem(hDlg, IDC_BTN_SAVE_SETTINGS), TRUE);
    EnableWindow(GetDlgItem(hDlg, IDC_BTN_RESET_SETTINGS), TRUE);
    EnableWindow(GetDlgItem(hDlg, IDC_BTN_LOAD_SETTINGS), TRUE);
    EnableWindow(GetDlgItem(hDlg, IDC_BTN_EXPORT_SETTINGS), TRUE);
}

std::string DataTab::GetDownloadsPath() {
    char path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_PROFILE, NULL, 0, path))) {
        std::string downloadsPath = std::string(path) + "\\Downloads";
        return downloadsPath;
    }
    return "C:\\Users\\Public\\Downloads"; // Fallback
}

std::string DataTab::GetSaveFilePath(bool useDownloads) {
    if (useDownloads) {
        // Generate timestamp for unique filename
        auto now = std::time(nullptr);
        auto tm = *std::localtime(&now);
        
        std::ostringstream oss;
        oss << GetDownloadsPath() << "\\UtilityApp_Settings_"
            << std::put_time(&tm, "%Y%m%d_%H%M%S") << ".config";
        return oss.str();
    } else {
        // Show save dialog
        OPENFILENAMEA ofn = {};
        char szFile[MAX_PATH] = "UtilityApp_Settings.config";
        
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = hTab;
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = sizeof(szFile);
        ofn.lpstrFilter = "Configuration Files (*.config)\0*.config\0Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0";
        ofn.nFilterIndex = 1;
        ofn.lpstrTitle = "Export Settings As...";
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;
        ofn.lpstrDefExt = "config";
        
        if (GetSaveFileNameA(&ofn)) {
            return std::string(szFile);
        }
    }
    return "";
}

std::string DataTab::GetLoadFilePath() {
    OPENFILENAMEA ofn = {};
    char szFile[MAX_PATH] = "";
    
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hTab;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "Configuration Files (*.config)\0*.config\0Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrTitle = "Load Settings From...";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
    
    if (GetOpenFileNameA(&ofn)) {
        return std::string(szFile);
    }
    return "";
}

bool DataTab::ConfirmReset() {
    int result = MessageBoxA(hTab,
        "Are you sure you want to reset all settings to default values?\n\n"
        "This action cannot be undone. All your current settings will be lost.\n\n"
        "Consider exporting your current settings first as a backup.",
        "Confirm Reset",
        MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2);
        
    return (result == IDYES);
}
