# Settings System Documentation

## Overview
The UtilityApp now includes a comprehensive settings system with a tabbed interface that allows users to configure various aspects of the application.

## Settings Categories

### 1. Lock & Input Tab
This tab provides configuration options for the core input locking functionality:

#### Input Types
- **Lock Keyboard**: Toggle to enable/disable keyboard locking
- **Lock Mouse**: Toggle to enable/disable mouse locking
- Users can choose one or both input types to lock when using the hotkey

#### Unlock Methods
Three unlock methods are available (radio button selection):
- **Password**: Unlock by typing a numeric password (default: 10203040)
- **Timer**: Unlock automatically after a specified time
- **Whitelist Keys**: Allow specific keys to pass through while locked

Each method has a "Configure..." button that opens detailed settings:
- Password: Change unlock password
- Timer: Set unlock duration
- Whitelist: Specify allowed keys

#### Hotkey Configuration
- **Lock Hotkey**: Currently set to Ctrl+Shift+I
- To edit: Click in the hotkey field, enter new combination, then Save or Cancel
- Visual feedback shows when editing is active

### 2. Productivity Tab
Currently shows "Features coming soon..." with planned features:
- USB Alert
- Clipboard Quick Actions  
- Quick Launch Apps
- Work/Break Timer
- Quick DND Mode
- Focus Mode

### 3. Privacy & Security Tab
Currently shows "Features coming soon..." with planned features:
- Clipboard Auto Clear
- Boss Key (Quick Hide)
- Recent Files Hub
- Secure File Deletion

### 4. Appearance Tab
Controls the visual overlay shown when input is locked:

#### Overlay Styles
- **Blur**: Apply a blur effect to the background
- **Dim**: Darken the background with transparency
- **Black Screen**: Show a solid black overlay
- **None**: No visual overlay (input still locked)

## Technical Implementation

### Architecture
- **settings.h/cpp**: Main settings system and dialog management
- **overlay.h/cpp**: Screen overlay system for visual feedback
- **settings_dialogs.rc**: Resource definitions for dialog layouts
- Integration with existing input_blocker and main application

### Key Features
- **Tabbed Interface**: Clean organization of settings categories
- **Unsaved Changes Detection**: Prompts user when switching tabs with unsaved changes
- **Real-time Preview**: Overlay changes take effect immediately
- **Settings Persistence**: Configuration saved to file (implementation ready)
- **Modular Design**: Easy to add new tabs and settings

### Dialog Controls
- **OK**: Save changes and close
- **Cancel**: Discard changes and close (with confirmation)
- **Apply**: Save changes without closing

### Settings Storage
The `AppSettings` structure contains all configuration options:
```cpp
struct AppSettings {
    // Lock & Input
    bool keyboardLockEnabled;
    bool mouseLockEnabled;
    int unlockMethod;
    string lockHotkey;
    
    // Unlock methods
    string unlockPassword;
    int timerDuration;
    string whitelistedKeys;
    
    // Appearance
    int overlayStyle;
};
```

## Usage Instructions

### Opening Settings
- Right-click system tray icon → "Settings..."
- Or right-click → "Change Hotkeys..." (opens to Lock & Input tab)

### Changing Lock Types
1. Open Settings → Lock & Input tab
2. Check/uncheck "Lock Keyboard" and "Lock Mouse"
3. Click Apply or OK to save

### Changing Unlock Method
1. Select desired radio button (Password/Timer/Whitelist)
2. Click "Configure..." to set specific options
3. Apply changes

### Editing Hotkeys
1. Click in the hotkey text field
2. Field clears and shows "Press the new hotkey combination..."
3. Press desired key combination
4. Click Save to confirm or Cancel to revert

### Changing Overlay Style
1. Open Settings → Appearance tab
2. Select desired overlay style
3. Changes take effect immediately when input is locked

## Future Enhancements

### Planned Features
- Complete implementation of Productivity and Privacy tabs
- Advanced hotkey conflict detection
- Settings import/export
- Multiple configuration profiles
- Keyboard shortcut for settings dialog

### Configuration Dialogs
- Password configuration: Change unlock password, enable/disable
- Timer configuration: Set duration, enable auto-unlock
- Whitelist configuration: Manage allowed keys during lock

## Troubleshooting

### Common Issues
- **Settings not saving**: Check file permissions in application directory
- **Hotkey conflicts**: Use unique key combinations
- **Overlay not showing**: Verify appearance settings and graphics drivers

### Debug Information
- Settings file location: (to be implemented)
- Log file location: (to be implemented)
- Registry keys used: (none currently)

## Development Notes

### Adding New Settings
1. Add fields to `AppSettings` structure
2. Create UI controls in appropriate tab resource
3. Add handling in tab dialog procedures
4. Update save/load functions

### Adding New Tabs
1. Define new tab resource ID in resource.h
2. Create dialog resource in settings_dialogs.rc
3. Add tab creation in `CreateTabDialogs()`
4. Implement tab dialog procedure
5. Add tab switch case in `SwitchTab()`

This modular approach makes the settings system easily extensible for future features.
