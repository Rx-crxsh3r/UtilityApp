// src/ui/productivity_tab.cpp
// Productivity Tab - OOP implementation

#include "productivity_tab.h"
#include "../settings.h"  // For SettingsDialog access
#include "../settings/settings_core.h"
#include "../features/productivity/productivity_manager.h"
#include "../resource.h"  // For control IDs
#include <commctrl.h>

// External references
extern ProductivityManager g_productivityManager;

// Static callback for Windows dialog system
INT_PTR CALLBACK ProductivityTab::DialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    ProductivityTab* tab = nullptr;

    if (message == WM_INITDIALOG) {
        tab = (ProductivityTab*)lParam;
        SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)tab);
        tab->SetDialogHandle(hDlg);
    } else {
        tab = (ProductivityTab*)GetWindowLongPtr(hDlg, GWLP_USERDATA);
    }

    if (tab) {
        return tab->HandleMessage(hDlg, message, wParam, lParam);
    }

    return FALSE;
}

ProductivityTab::ProductivityTab(SettingsDialog* parent, AppSettings* settings, bool* unsavedFlag)
    : parentDialog(parent), tempSettings(settings), hasUnsavedChanges(unsavedFlag),
      hTabDialog(nullptr) {
}

ProductivityTab::~ProductivityTab() {
    // Clean up font resources if any
    if (hTabDialog) {
        HFONT hFont = (HFONT)GetProp(hTabDialog, TEXT("DialogFont"));
        if (hFont) {
            DeleteObject(hFont);
            RemoveProp(hTabDialog, TEXT("DialogFont"));
        }
    }
}

INT_PTR ProductivityTab::HandleMessage(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_INITDIALOG: {
            hTabDialog = hDlg;
            InitializeControls();
            return TRUE;
        }

        case WM_COMMAND: {
            HandleControlCommand(wParam, lParam);
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

void ProductivityTab::InitializeControls() {
    if (!hTabDialog || !tempSettings) return;

    // Initialize productivity controls from current settings
    CheckDlgButton(hTabDialog, IDC_CHECK_USB_ALERT, tempSettings->usbAlertEnabled ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hTabDialog, IDC_CHECK_QUICK_LAUNCH, tempSettings->quickLaunchEnabled ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hTabDialog, IDC_CHECK_TIMER, tempSettings->workBreakTimerEnabled ? BST_CHECKED : BST_UNCHECKED);

    // Initialize boss key controls
    CheckDlgButton(hTabDialog, IDC_CHECK_BOSS_KEY, tempSettings->bossKeyEnabled ? BST_CHECKED : BST_UNCHECKED);
    SetDlgItemTextA(hTabDialog, IDC_EDIT_HOTKEY_BOSS, tempSettings->bossKeyHotkey.c_str());
    EnableWindow(GetDlgItem(hTabDialog, IDC_EDIT_HOTKEY_BOSS), tempSettings->bossKeyEnabled);
    EnableWindow(GetDlgItem(hTabDialog, IDC_BTN_BOSS_KEY_TEST), tempSettings->bossKeyEnabled);
}

void ProductivityTab::HandleControlCommand(WPARAM wParam, LPARAM lParam) {
    switch (LOWORD(wParam)) {
        case IDC_CHECK_USB_ALERT: {
            bool oldValue = tempSettings->usbAlertEnabled;
            tempSettings->usbAlertEnabled = (IsDlgButtonChecked(hTabDialog, IDC_CHECK_USB_ALERT) == BST_CHECKED);

            if (oldValue != tempSettings->usbAlertEnabled) {
                *hasUnsavedChanges = true;
            }
            break;
        }

        case IDC_CHECK_QUICK_LAUNCH: {
            bool oldValue = tempSettings->quickLaunchEnabled;
            tempSettings->quickLaunchEnabled = (IsDlgButtonChecked(hTabDialog, IDC_CHECK_QUICK_LAUNCH) == BST_CHECKED);

            if (oldValue != tempSettings->quickLaunchEnabled) {
                *hasUnsavedChanges = true;
            }
            break;
        }

        case IDC_CHECK_TIMER: {
            bool oldValue = tempSettings->workBreakTimerEnabled;
            tempSettings->workBreakTimerEnabled = (IsDlgButtonChecked(hTabDialog, IDC_CHECK_TIMER) == BST_CHECKED);

            if (oldValue != tempSettings->workBreakTimerEnabled) {
                *hasUnsavedChanges = true;
            }
            break;
        }

        case IDC_BTN_TIMER_CONFIG:
            ShowTimerConfig();
            break;

        case IDC_BTN_QUICK_LAUNCH_CONFIG:
            ShowQuickLaunchConfig();
            break;

        case IDC_BTN_START_WORK_SESSION:
            StartWorkSession();
            break;
    }
}

void ProductivityTab::RefreshControls() {
    if (hTabDialog) {
        // Re-initialize the tab with current tempSettings values
        InitializeControls();
    }
}

void ProductivityTab::ShowTimerConfig() {
    if (!hTabDialog) return;

    MessageBoxA(hTabDialog, "Timer Configuration:\n\nWork Duration: 25 minutes\nShort Break: 5 minutes\nLong Break: 15 minutes\n\n(Advanced configuration coming in next update)",
               "Pomodoro Timer Settings", MB_OK | MB_ICONINFORMATION);
}

void ProductivityTab::ShowQuickLaunchConfig() {
    if (!hTabDialog) return;

    MessageBoxA(hTabDialog, "Quick Launch Configuration:\n\nDefault hotkeys:\nCtrl+F1 - Notepad\nCtrl+F2 - Calculator\nCtrl+Alt+F3 - File Explorer\n\n(Custom app configuration coming in next update)",
               "Quick Launch Settings", MB_OK | MB_ICONINFORMATION);
}

void ProductivityTab::StartWorkSession() {
    if (!hTabDialog) return;

    if (tempSettings->workBreakTimerEnabled) {
        if (g_productivityManager.StartWorkSession()) {
            MessageBoxA(hTabDialog, "Work session started! You'll be notified when it's time for a break.\n\nTimer: 25 minutes work, 5 minute breaks\nLong break every 4 sessions",
                       "Pomodoro Timer", MB_OK | MB_ICONINFORMATION);
        } else {
            MessageBoxA(hTabDialog, "Failed to start work session. Make sure the timer feature is enabled and applied.",
                       "Error", MB_OK | MB_ICONERROR);
        }
    } else {
        MessageBoxA(hTabDialog, "Please enable the Work/Break Timer feature first, then click Apply.",
                   "Timer Not Enabled", MB_OK | MB_ICONWARNING);
    }
}
