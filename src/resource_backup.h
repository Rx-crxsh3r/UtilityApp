1:// src/resource.h
2:
3:#pragma once
4:
5:// Icon Resource
6:#define IDI_APPICON 101
7:
8:// System Tray Context Menu Resources
9:#define IDM_TRAY_MENU         102
10:#define IDM_LOCK_UNLOCK       103
11:#define IDM_SETTINGS          104
12:#define IDM_CHANGE_HOTKEYS    105
13:#define IDM_CHANGE_PASSWORD   106
14:#define IDM_ABOUT             107
15:#define IDM_EXIT              108
16:
17:// Custom Window Messages
18:#define WM_TRAY_ICON_MSG (WM_USER + 1)
19:
20:// Hotkey IDs
21:#define HOTKEY_ID_LOCK 1
22:#define HOTKEY_ID_UNLOCK 2
23:
24:// Settings Dialog Resource IDs
25:#define IDD_SETTINGS_DIALOG     200
26:#define IDC_TAB_CONTROL         201
27:
28:// Tab Resource IDs  
29:#define IDD_TAB_LOCK_INPUT      210
30:#define IDD_TAB_PRODUCTIVITY    211
31:#define IDD_TAB_PRIVACY         212
32:#define IDD_TAB_APPEARANCE      213
33:
34:// Lock & Input Tab Controls
35:#define IDC_CHECK_KEYBOARD      220
36:#define IDC_CHECK_MOUSE         221
37:#define IDC_RADIO_PASSWORD      222
38:#define IDC_RADIO_TIMER         223
39:#define IDC_CHECK_WHITELIST     224
40:#define IDC_EDIT_HOTKEY_LOCK    225
41:#define IDC_BTN_PASSWORD_CFG    226
42:#define IDC_BTN_TIMER_CFG       227
43:#define IDC_BTN_WHITELIST_CFG   228
44:#define IDC_BTN_SAVE_HOTKEY     229
45:#define IDC_BTN_CANCEL_HOTKEY   230
46:#define IDC_LABEL_HOTKEY_HINT   231
47:#define IDC_LABEL_HOTKEY_WARNING 232
48:#define IDC_CHECK_UNLOCK_HOTKEY 233
49:#define IDC_EDIT_UNLOCK_HOTKEY  234
50:
51:// Warning Labels
52:#define IDC_WARNING_KEYBOARD_UNLOCK     280
53:#define IDC_WARNING_LOCKING_DISABLED    281
54:#define IDC_WARNING_SINGLE_KEY          282
55:
56:// Timer Tab Controls
57:#define IDC_RADIO_TIMER_DISABLED    232
58:#define IDC_RADIO_TIMER_PERIODIC    233
59:#define IDC_EDIT_TIMER_DURATION     234
60:#define IDC_EDIT_TIMER_INTERVAL     235
61:#define IDC_LABEL_TIMER_STATUS      236
62:
63:// Password Tab Controls
64:#define IDC_EDIT_PASSWORD           237
65:#define IDC_BUTTON_CLEAR_PASSWORD   238
66:
67:// Command IDs
68:#define ID_LOCK_INPUT               270
69:#define ID_UNLOCK_INPUT             271
70:#define ID_TOGGLE_INPUT             272
71:
72:// Appearance Tab Controls
73:#define IDC_RADIO_BLUR          240
74:#define IDC_RADIO_DIM           241
75:#define IDC_RADIO_BLACK         242
76:#define IDC_RADIO_NONE          243
77:#define IDC_LABEL_OVERLAY_DESC  244
78:
79:// Notification Style Controls
80:#define IDC_RADIO_NOTIFY_CUSTOM   245
81:#define IDC_RADIO_NOTIFY_WINDOWS  246
82:#define IDC_RADIO_NOTIFY_NONE     247
83:#define IDC_LABEL_NOTIFY_DESC     248
84:
85:// Placeholder Controls for Coming Soon tabs
86:#define IDC_LABEL_COMING_SOON   250
87:
88:// Productivity Tab Controls
89:#define IDC_CHECK_USB_ALERT           290
90:#define IDC_CHECK_QUICK_LAUNCH        291
91:#define IDC_CHECK_TIMER               292
92:#define IDC_BTN_TIMER_CONFIG          294
93:#define IDC_BTN_QUICK_LAUNCH_CONFIG   295
94:#define IDC_BTN_START_WORK_SESSION    296
95:
96:// Privacy Tab Controls
97:#define IDC_CHECK_HIDE_TASKBAR        300
98:#define IDC_CHECK_START_WINDOWS       302
99:#define IDC_CHECK_BOSS_KEY            303
100:#define IDC_BTN_BOSS_KEY_TEST         304
101:#define IDC_BTN_BOSS_KEY_CHANGE       305
102:#define IDC_EDIT_HOTKEY_BOSS          306
103:#define IDC_CHECK_UNLOCK_HOTKEY       307
104:#define IDC_EDIT_UNLOCK_HOTKEY        308
105:#define IDC_BTN_TEST_UNLOCK           309
106:
107:// Button Controls
108:#define IDC_BTN_OK              260
109:#define IDC_BTN_CANCEL          261
110:#define IDC_BTN_APPLY           262