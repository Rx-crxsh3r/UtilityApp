// src/tray_icon.h

#pragma once
#include <windows.h>

// Adds the application icon to the system tray.
void AddTrayIcon(HWND hwnd);

// Removes the application icon from the system tray.
void RemoveTrayIcon(HWND hwnd);

// Shows the context menu when the tray icon is right-clicked.
void ShowContextMenu(HWND hwnd);