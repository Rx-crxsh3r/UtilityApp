// src/ui/privacy_tab.cpp
// Privacy tab implementation - OOP implementation

#include "privacy_tab.h"
#include "../settings.h"
#include "../resource.h"
#include "../features/privacy/privacy_manager.h"
#include "../settings/hotkey_manager.h"
#include <commctrl.h>

// Global manager instances
extern PrivacyManager g_privacyManager;
extern HotkeyManager g_hotkeyManager;

PrivacyTab::PrivacyTab(SettingsDialog* parent, AppSettings* settings, bool* unsavedChanges)
    : parentDialog(parent), tempSettings(settings), hasUnsavedChanges(unsavedChanges), hTab(nullptr) {
}

PrivacyTab::~PrivacyTab() {
    // Clean up will be handled by parent dialog
}

INT_PTR CALLBACK PrivacyTab::DialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    PrivacyTab* tab = nullptr;

    if (message == WM_INITDIALOG) {
        tab = (PrivacyTab*)lParam;
        SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)tab);
        tab->hTab = hDlg;
    } else {
        tab = (PrivacyTab*)GetWindowLongPtr(hDlg, GWLP_USERDATA);
    }

    if (!tab) return FALSE;

    return tab->HandleMessage(hDlg, message, wParam, lParam);
}

INT_PTR PrivacyTab::HandleMessage(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_INITDIALOG: {
            InitializeControls(hDlg);
            return TRUE;
        }

        case WM_COMMAND: {
            switch (LOWORD(wParam)) {
                case IDC_CHECK_START_WINDOWS:
                    OnStartWithWindowsChanged(hDlg);
                    break;

                case IDC_CHECK_BOSS_KEY:
                    OnBossKeyEnabledChanged(hDlg);
                    break;

                case IDC_EDIT_HOTKEY_BOSS:
                    if (HIWORD(wParam) == EN_SETFOCUS) {
                        OnBossKeyHotkeyFocus(hDlg);
                    }
                    break;

                case IDC_BTN_BOSS_KEY_TEST:
                    OnBossKeyTestClicked(hDlg);
                    break;
            }
            break;
        }

        case WM_DESTROY: {
            // Clean up font resources
            HFONT hFont = (HFONT)GetProp(hDlg, TEXT("DialogFont"));
            if (hFont) {
                DeleteObject(hFont);
                RemoveProp(hDlg, TEXT("DialogFont"));
            }
            return TRUE;
        }
    }

    return FALSE;
}

void PrivacyTab::InitializeControls(HWND hDlg) {
    // Initialize start with Windows checkbox
    CheckDlgButton(hDlg, IDC_CHECK_START_WINDOWS,
                   tempSettings->startWithWindows ? BST_CHECKED : BST_UNCHECKED);

    // Initialize boss key controls
    CheckDlgButton(hDlg, IDC_CHECK_BOSS_KEY,
                   tempSettings->bossKeyEnabled ? BST_CHECKED : BST_UNCHECKED);
    SetDlgItemTextA(hDlg, IDC_EDIT_HOTKEY_BOSS, tempSettings->bossKeyHotkey.c_str());

    // Update boss key control states
    UpdateBossKeyControls(hDlg);
}

void PrivacyTab::RefreshControls() {
    if (!hTab) return;

    // Re-initialize all controls with current tempSettings values
    InitializeControls(hTab);
}

void PrivacyTab::OnStartWithWindowsChanged(HWND hDlg) {
    bool oldValue = tempSettings->startWithWindows;
    tempSettings->startWithWindows = (IsDlgButtonChecked(hDlg, IDC_CHECK_START_WINDOWS) == BST_CHECKED);

    if (oldValue != tempSettings->startWithWindows) {
        *hasUnsavedChanges = true;
    }
}

void PrivacyTab::OnBossKeyEnabledChanged(HWND hDlg) {
    bool oldValue = tempSettings->bossKeyEnabled;
    tempSettings->bossKeyEnabled = (IsDlgButtonChecked(hDlg, IDC_CHECK_BOSS_KEY) == BST_CHECKED);

    // Update control states
    UpdateBossKeyControls(hDlg);

    if (oldValue != tempSettings->bossKeyEnabled) {
        *hasUnsavedChanges = true;
    }
}

void PrivacyTab::OnBossKeyHotkeyFocus(HWND hDlg) {
    if (tempSettings->bossKeyEnabled) {
        // User clicked boss key textbox - start capture
        HWND hEdit = GetDlgItem(hDlg, IDC_EDIT_HOTKEY_BOSS);
        g_hotkeyManager.StartCapture(hDlg, hEdit, nullptr, tempSettings->bossKeyHotkey);
    }
}

void PrivacyTab::OnBossKeyTestClicked(HWND hDlg) {
    if (g_privacyManager.IsBossKeyActive()) {
        g_privacyManager.DeactivateBossKey();
        MessageBoxA(hDlg, "Boss Key deactivated! All windows have been restored.",
                   "Boss Key Test", MB_OK | MB_ICONINFORMATION);
    } else {
        MessageBoxA(hDlg, "Testing Boss Key... All windows will be hidden for 3 seconds!",
                   "Boss Key Test", MB_OK | MB_ICONINFORMATION);
        g_privacyManager.ActivateBossKey();
        Sleep(3000);
        g_privacyManager.DeactivateBossKey();
    }
}

void PrivacyTab::UpdateBossKeyControls(HWND hDlg) {
    bool enabled = tempSettings->bossKeyEnabled;

    EnableWindow(GetDlgItem(hDlg, IDC_EDIT_HOTKEY_BOSS), enabled);
    EnableWindow(GetDlgItem(hDlg, IDC_BTN_BOSS_KEY_TEST), enabled);
}
