// src/tray_icon.cpp

#include <cstring>
#include "tray_icon.h"
#include "resource.h"
#include "input_blocker.h"

void AddTrayIcon(HWND hwnd) {
    NOTIFYICONDATAA nid = {};
    nid.cbSize = sizeof(NOTIFYICONDATAA);
    nid.hWnd = hwnd;
    nid.uID = 1;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAY_ICON_MSG;
    nid.hIcon = (HICON)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_APPICON), IMAGE_ICON, 
                                GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
    strcpy_s(nid.szTip, "UtilityApp");

    Shell_NotifyIconA(NIM_ADD, &nid);
}

void RemoveTrayIcon(HWND hwnd) {
    NOTIFYICONDATAA nid = {};
    nid.cbSize = sizeof(NOTIFYICONDATAA);
    nid.hWnd = hwnd;
    nid.uID = 1;

    Shell_NotifyIconA(NIM_DELETE, &nid);
}

void ShowContextMenu(HWND hwnd) {
    POINT pt;
    GetCursorPos(&pt);

    HMENU hMenu = LoadMenuA(GetModuleHandle(NULL), MAKEINTRESOURCE(IDM_TRAY_MENU));
    if (hMenu) {
        HMENU hSubMenu = GetSubMenu(hMenu, 0);
        if (hSubMenu) {
            // Set foreground window to handle menu dismissal correctly
            SetForegroundWindow(hwnd);

            // Update menu item text based on lock state
            UINT uFlags = IsInputLocked() ? MF_STRING | MF_CHECKED : MF_STRING | MF_UNCHECKED;
            ModifyMenuA(hSubMenu, IDM_LOCK_UNLOCK, uFlags, IDM_LOCK_UNLOCK, IsInputLocked() ? "Unlock Input" : "Lock Input");
            
            // Display the menu
            TrackPopupMenu(hSubMenu, TPM_LEFTALIGN | TPM_BOTTOMALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, NULL);
        }
        DestroyMenu(hMenu);
    }
}