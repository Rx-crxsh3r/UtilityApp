// src/resource.h

#pragma once

// Icon Resource
#define IDI_APPICON 101

// System Tray Context Menu Resources
#define IDM_TRAY_MENU         102
#define IDM_LOCK_UNLOCK       103
#define IDM_SETTINGS          104
#define IDM_CHANGE_HOTKEYS    105
#define IDM_CHANGE_PASSWORD   106
#define IDM_ABOUT             107
#define IDM_EXIT              108

// Custom Window Messages
#define WM_TRAY_ICON_MSG (WM_USER + 1)

// Hotkey IDs
#define HOTKEY_ID_LOCK 1
#define HOTKEY_ID_UNLOCK 2

// Settings Dialog Resource IDs
#define IDD_SETTINGS_DIALOG     200
#define IDC_TAB_CONTROL         201

// Tab Resource IDs  
#define IDD_TAB_LOCK_INPUT      210
#define IDD_TAB_PRODUCTIVITY    211
#define IDD_TAB_PRIVACY         212
#define IDD_TAB_APPEARANCE      213

// Lock & Input Tab Controls
#define IDC_CHECK_KEYBOARD      220
#define IDC_CHECK_MOUSE         221
#define IDC_RADIO_PASSWORD      222
#define IDC_RADIO_TIMER         223
#define IDC_RADIO_WHITELIST     224
#define IDC_EDIT_HOTKEY_LOCK    225
#define IDC_BTN_PASSWORD_CFG    226
#define IDC_BTN_TIMER_CFG       227
#define IDC_BTN_WHITELIST_CFG   228
#define IDC_BTN_SAVE_HOTKEY     229
#define IDC_BTN_CANCEL_HOTKEY   230
#define IDC_LABEL_HOTKEY_HINT   231

// Warning Labels
#define IDC_WARNING_KEYBOARD_UNLOCK     280
#define IDC_WARNING_LOCKING_DISABLED    281
#define IDC_WARNING_SINGLE_KEY          282

// New Unlock Hotkey Controls
#define IDC_CHECK_UNLOCK_HOTKEY         283
#define IDC_EDIT_UNLOCK_HOTKEY          284
#define IDC_LABEL_UNLOCK_HOTKEY         285

// Timer Tab Controls
#define IDC_RADIO_TIMER_DISABLED    232
#define IDC_RADIO_TIMER_PERIODIC    233
#define IDC_EDIT_TIMER_DURATION     234
#define IDC_EDIT_TIMER_INTERVAL     235
#define IDC_LABEL_TIMER_STATUS      236

// Password Tab Controls
#define IDC_EDIT_PASSWORD           237
#define IDC_BUTTON_CLEAR_PASSWORD   238

// Command IDs
#define ID_LOCK_INPUT               270
#define ID_UNLOCK_INPUT             271
#define ID_TOGGLE_INPUT             272

// Appearance Tab Controls
#define IDC_RADIO_BLUR          240
#define IDC_RADIO_DIM           241
#define IDC_RADIO_BLACK         242
#define IDC_RADIO_NONE          243
#define IDC_LABEL_OVERLAY_DESC  244

// Notification Style Controls
#define IDC_RADIO_NOTIFY_CUSTOM   245
#define IDC_RADIO_NOTIFY_WINDOWS  246
#define IDC_RADIO_NOTIFY_NONE     247
#define IDC_LABEL_NOTIFY_DESC     248

// Placeholder Controls for Coming Soon tabs
#define IDC_LABEL_COMING_SOON   250

// Productivity Tab Controls
#define IDC_CHECK_USB_ALERT           290
#define IDC_CHECK_QUICK_LAUNCH        291
#define IDC_CHECK_TIMER               292
#define IDC_BTN_TIMER_CONFIG          294
#define IDC_BTN_QUICK_LAUNCH_CONFIG   295
#define IDC_BTN_START_WORK_SESSION    296

// Privacy Tab Controls
#define IDC_CHECK_HIDE_TASKBAR        300
#define IDC_CHECK_START_WINDOWS       302
#define IDC_CHECK_BOSS_KEY            303
#define IDC_BTN_BOSS_KEY_TEST         304
#define IDC_BTN_BOSS_KEY_CHANGE       305
#define IDC_EDIT_HOTKEY_BOSS          306

// Button Controls
#define IDC_BTN_OK              260
#define IDC_BTN_CANCEL          261
#define IDC_BTN_APPLY           262