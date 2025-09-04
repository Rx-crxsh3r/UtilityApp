// src/ui/appearance_tab.cpp
// Appearance tab implementation - OOP implementation

#include "appearance_tab.h"
#include "../settings.h"
#include "../resource.h"
#include "../settings/overlay_manager.h"
#include "../custom_notifications.h"
#include <commctrl.h>

// Global manager instances
extern OverlayManager g_overlayManager;
extern CustomNotificationSystem* g_customNotifications;

AppearanceTab::AppearanceTab(SettingsDialog* parent, AppSettings* settings, bool* unsavedChanges)
    : parentDialog(parent), tempSettings(settings), hasUnsavedChanges(unsavedChanges), hTab(nullptr) {
}

AppearanceTab::~AppearanceTab() {
    // Clean up will be handled by parent dialog
}

INT_PTR CALLBACK AppearanceTab::DialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    AppearanceTab* tab = nullptr;

    if (message == WM_INITDIALOG) {
        tab = (AppearanceTab*)lParam;
        SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)tab);
        tab->hTab = hDlg;
    } else {
        tab = (AppearanceTab*)GetWindowLongPtr(hDlg, GWLP_USERDATA);
    }

    if (!tab) return FALSE;

    return tab->HandleMessage(hDlg, message, wParam, lParam);
}

INT_PTR AppearanceTab::HandleMessage(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_INITDIALOG: {
            InitializeControls(hDlg);
            return TRUE;
        }

        case WM_COMMAND: {
            switch (LOWORD(wParam)) {
                case IDC_RADIO_BLUR:
                case IDC_RADIO_DIM:
                case IDC_RADIO_BLACK:
                case IDC_RADIO_NONE:
                    OnOverlayStyleChanged(hDlg, wParam);
                    break;

                case IDC_RADIO_NOTIFY_CUSTOM:
                case IDC_RADIO_NOTIFY_WINDOWS:
                case IDC_RADIO_NOTIFY_WINDOWS_NOTIF:
                case IDC_RADIO_NOTIFY_NONE:
                    OnNotificationStyleChanged(hDlg, wParam);
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

void AppearanceTab::InitializeControls(HWND hDlg) {
    // Initialize overlay manager with current settings
    g_overlayManager.SetStyle((OverlayStyle)tempSettings->overlayStyle);
    g_overlayManager.InitializeRadioButtons(hDlg, IDC_RADIO_BLUR);

    // Set overlay description
    SetDlgItemTextA(hDlg, IDC_LABEL_OVERLAY_DESC,
                   "Choose the overlay style that appears when input is locked:");

    // Set notification description
    SetDlgItemTextA(hDlg, IDC_LABEL_NOTIFY_DESC,
                   "Choose notification style for system alerts:");

    // Initialize notification style radio buttons
    UpdateNotificationStyleRadios(hDlg);
}

void AppearanceTab::RefreshControls() {
    if (!hTab) return;

    // Re-initialize all controls with current tempSettings values
    InitializeControls(hTab);
}

void AppearanceTab::OnOverlayStyleChanged(HWND hDlg, WPARAM wParam) {
    // Use modular overlay manager
    int oldStyle = tempSettings->overlayStyle;
    g_overlayManager.HandleRadioButtonClick(hDlg, LOWORD(wParam), IDC_RADIO_BLUR);
    tempSettings->overlayStyle = g_overlayManager.GetStyle();

    if (oldStyle != tempSettings->overlayStyle) {
        *hasUnsavedChanges = true;
    }
}

void AppearanceTab::OnNotificationStyleChanged(HWND hDlg, WPARAM wParam) {
    int oldNotifyStyle = tempSettings->notificationStyle;

    if (LOWORD(wParam) == IDC_RADIO_NOTIFY_CUSTOM) {
        tempSettings->notificationStyle = 0;
        CheckRadioButton(hDlg, IDC_RADIO_NOTIFY_CUSTOM, IDC_RADIO_NOTIFY_NONE, IDC_RADIO_NOTIFY_CUSTOM);
    } else if (LOWORD(wParam) == IDC_RADIO_NOTIFY_WINDOWS) {
        tempSettings->notificationStyle = 1;
        CheckRadioButton(hDlg, IDC_RADIO_NOTIFY_CUSTOM, IDC_RADIO_NOTIFY_NONE, IDC_RADIO_NOTIFY_WINDOWS);
    } else if (LOWORD(wParam) == IDC_RADIO_NOTIFY_WINDOWS_NOTIF) {
        tempSettings->notificationStyle = 2;
        CheckRadioButton(hDlg, IDC_RADIO_NOTIFY_CUSTOM, IDC_RADIO_NOTIFY_NONE, IDC_RADIO_NOTIFY_WINDOWS_NOTIF);
    } else if (LOWORD(wParam) == IDC_RADIO_NOTIFY_NONE) {
        tempSettings->notificationStyle = 3;
        CheckRadioButton(hDlg, IDC_RADIO_NOTIFY_CUSTOM, IDC_RADIO_NOTIFY_NONE, IDC_RADIO_NOTIFY_NONE);
    }

    // Update notification system style
    if (g_customNotifications) {
        g_customNotifications->SetStyle((NotificationStyle)tempSettings->notificationStyle);
    }

    if (oldNotifyStyle != tempSettings->notificationStyle) {
        *hasUnsavedChanges = true;
    }
}

void AppearanceTab::UpdateNotificationStyleRadios(HWND hDlg) {
    switch (tempSettings->notificationStyle) {
        case 0: // Custom
            CheckRadioButton(hDlg, IDC_RADIO_NOTIFY_CUSTOM, IDC_RADIO_NOTIFY_NONE, IDC_RADIO_NOTIFY_CUSTOM);
            break;
        case 1: // Windows
            CheckRadioButton(hDlg, IDC_RADIO_NOTIFY_CUSTOM, IDC_RADIO_NOTIFY_NONE, IDC_RADIO_NOTIFY_WINDOWS);
            break;
        case 2: // Windows Notifications
            CheckRadioButton(hDlg, IDC_RADIO_NOTIFY_CUSTOM, IDC_RADIO_NOTIFY_NONE, IDC_RADIO_NOTIFY_WINDOWS_NOTIF);
            break;
        case 3: // None
            CheckRadioButton(hDlg, IDC_RADIO_NOTIFY_CUSTOM, IDC_RADIO_NOTIFY_NONE, IDC_RADIO_NOTIFY_NONE);
            break;
    }
}
