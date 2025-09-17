# UtilityApp

A lightweight, high-performance Windows utility application written in C++ using the Win32 API that runs silently in the background and provides global input locking capabilities.  Provides advanced input locking, productivity tracking, privacy controls, and system monitoring with a clean, modular architecture.

## 🎯 Features

### 🔒 Input Control System
- **Global Input Locking**: Lock/unlock keyboard and mouse system-wide
- **Multiple Unlock Methods**:
  - Password-based unlock (secure SHA-256 hashing)
  - Timer-based auto-unlock
  - Whitelist-based selective unlocking
- **Failsafe Mechanism**: Emergency exit using ESC key sequence
- **Visual Feedback**: Multiple screen overlay styles (blur, dim, black)

### 🚀 Productivity Enhancement
- **USB Device Monitoring**: Real-time alerts for device insertion/removal
- **Quick Launch System**: Custom hotkey shortcuts for applications (Ctrl+F1-F12)
- **Pomodoro Timer**: Work/break cycle management (25min work, 5min break, 15min long break)
- **Session Tracking**: Automated productivity session management

### 🔐 Privacy & Security
- **Boss Key**: Instant window minimization/hiding (configurable hotkey)
- **System Integration**: Startup with Windows, taskbar visibility control
- **Secure Settings**: Registry-based persistence with integrity validation

### 🎨 User Interface
- **System Tray Integration**: Clean notification area interface
- **Advanced Settings Dialog**: Tabbed interface for all features
- **Multiple Notification Styles**:
  - Custom overlay notifications
  - Windows system notifications
  - Balloon tooltips
- **Settings Import/Export**: Cross-platform configuration backup

## 🔑 Hotkeys & Controls

### Core Input Controls
| Action | Default Hotkey | Alternative |
|--------|----------------|-------------|
| **Lock Input** | `Ctrl + Shift + L` | Tray icon → Lock/Unlock |
| **Unlock Input** | Password: `10203040` | Timer/Whitelist (configurable) |
| **Emergency Exit** | `ESC` × 3 (within 3s) | Tray icon → Exit |
| **Quick Toggle** | Double-click tray icon | - |

### Productivity Features
| Feature | Hotkey Range | Description |
|---------|--------------|-------------|
| **Quick Launch** | `Ctrl + F1-F12` | Launch assigned applications |
| **Boss Key** | `Ctrl + Alt + F12` | Hide all windows instantly |

### Tray Icon Menu
- **Lock/Unlock Input**: Toggle input blocking
- **Settings**: Open configuration dialog
- **Start Work Session**: Begin Pomodoro timer
- **Exit**: Close application

## 🚀 Getting Started

### System Requirements
- **OS**: Windows 10/11 (64-bit)
- **Compiler**: MinGW-w64 GCC (UCRT runtime)
- **Build Tools**: Windows SDK, Resource Compiler
- **RAM**: < 10MB (idle), < 15MB (active)
- **Storage**: ~2MB for application and settings

### Quick Start

1. **Download**: Get the latest `UtilityApp.exe` from releases
2. **Run**: Double-click to start (runs in background)
3. **Configure**: Right-click tray icon → Settings
4. **Lock Input**: Press `Ctrl + Shift + L` or use tray menu

### First-Time Setup

1. **Launch Application**: UtilityApp starts minimized to system tray
2. **Access Settings**: Right-click tray icon → Settings
3. **Configure Lock Method**: Choose password, timer, or whitelist unlock
4. **Set Hotkeys**: Customize lock/unlock combinations
5. **Enable Features**: Configure productivity and privacy options
6. **Test Lock**: Use tray menu or hotkey to test input blocking

## 📁 Project Structure

```
UtilityApp/
├── src/
│   ├── main.cpp                    # Application entry point
│   ├── input_blocker.cpp/.h        # Input locking system
│   ├── tray_icon.cpp/.h            # System tray integration
│   ├── failsafe.cpp/.h             # Emergency unlock system
│   ├── overlay.cpp/.h              # Screen overlay effects
│   ├── notifications.cpp/.h        # System notifications
│   ├── custom_notifications.cpp/.h # Custom overlay notifications
│   ├── audio_manager.cpp/.h        # Sound system
│   ├── settings.cpp/.h             # Settings dialog
│   ├── settings_core.cpp/.h        # Registry persistence
│   ├── utils/
│   │   └── hotkey_utils.cpp/.h     # Hotkey parsing utilities
│   ├── features/
│   │   ├── productivity/
│   │   │   └── productivity_manager.cpp/.h  # Productivity features
│   │   └── privacy/
│   │       └── privacy_manager.cpp/.h       # Privacy controls
│   └── ui/
│       ├── lock_input_tab.cpp/.h    # Lock & input settings
│       ├── productivity_tab.cpp/.h  # Productivity UI
│       └── privacy_tab.cpp/.h       # Privacy UI
├── resources/
│   ├── icon.ico                    # Application icon
│   ├── resources.rc                # Resource definitions
│   └── notif.wav                   # Notification sound
├── docs/
│   ├──                             # Empty folder <Placeholder for docs files>
├── build.bat                       # Automated build script
├── clean.bat                       # Build cleanup script
└── README.md                       # This file
```

## 🛠️ Building from Source

### Prerequisites
- **MinGW-w64 GCC** (with UCRT runtime)
- **Windows SDK** (for resource compilation)
- **Git** (for cloning)

### Automated Build (Recommended)

```batch
# Clone and build
git clone https://github.com/Rx-crxsh3r/UtilityApp
cd UtilityApp
build.bat
```

### Manual Build Steps

1. **Setup Environment**
   ```batch
   cd UtilityApp
   mkdir build
   ```

2. **Compile Resources**
   ```batch
   windres resources\resources.rc -o build\resources.o
   ```

3. **Compile Core Modules**
   ```batch
   gcc -c src\main.cpp -o build\main.o
   gcc -c src\tray_icon.cpp -o build\tray_icon.o
   gcc -c src\input_blocker.cpp -o build\input_blocker.o
   gcc -c src\failsafe.cpp -o build\failsafe.o
   gcc -c src\notifications.cpp -o build\notifications.o
   gcc -c src\custom_notifications.cpp -o build\custom_notifications.o
   gcc -c src\overlay.cpp -o build\overlay.o
   gcc -c src\audio_manager.cpp -o build\audio_manager.o
   ```

4. **Compile Settings System**
   ```batch
   gcc -c src\settings.cpp -o build\settings.o
   gcc -c src\settings_core.cpp -o build\settings_core.o
   gcc -c src\utils\hotkey_utils.cpp -o build\hotkey_utils.o
   ```

5. **Compile Feature Modules**
   ```batch
   gcc -c src\features\productivity\productivity_manager.cpp -o build\productivity_manager.o
   gcc -c src\features\privacy\privacy_manager.cpp -o build\privacy_manager.o
   gcc -c src\ui\lock_input_tab.cpp -o build\lock_input_tab.o
   gcc -c src\ui\productivity_tab.cpp -o build\productivity_tab.o
   gcc -c src\ui\privacy_tab.cpp -o build\privacy_tab.o
   ```

6. **Link Executable**
   ```batch
   gcc build\*.o -o UtilityApp.exe -luser32 -lkernel32 -lshell32 -ldwmapi -ladvapi32 -lgdi32 -lcomctl32
   ```

## ⚙️ Configuration Guide

### Basic Settings
- **Lock Hotkey**: Default `Ctrl + Shift + L`
- **Unlock Password**: Default `10203040`
- **Failsafe**: `ESC` × 3 within 3 seconds
- **Overlay Style**: Dim (50% transparency)

### Advanced Features

#### Productivity Configuration
- **USB Alerts**: Enable/disable device connection notifications
- **Quick Launch**: Assign applications to `Ctrl+F1-F12`
- **Pomodoro Timer**: Configure work/break durations

#### Privacy Settings
- **Boss Key**: Set custom hotkey for instant hide
- **Startup Integration**: Launch with Windows
- **Taskbar Visibility**: Control window appearance

#### Appearance Options
- **Notification Style**: Custom overlay, Windows notifications, or none
- **Overlay Effects**: Blur (Aero), dim, or solid black
- **Sound Effects**: Enable/disable notification sounds

### Settings Import/Export

```bash
# Export current settings
# Use Settings Dialog → Data Management → Export

# Import settings from file
# Use Settings Dialog → Data Management → Import
```

**Export Format**: INI-style text file with all configuration options

## 🔧 Technical Specifications

### Architecture
- **3-Layer System**: Data Persistence → Feature Management → User Interface
- **Modular Design**: Independent feature managers with clean separation
- **Registry Persistence**: `HKCU\SOFTWARE\UtilityApp` with integrity validation
- **Message-Driven**: Windows message pump with hook integration

### Performance Metrics
- **Memory Usage**: < 10MB RAM (idle), < 15MB (active)
- **CPU Usage**: ~0% (idle), < 1% (input processing)
- **Startup Time**: < 200ms
- **Registry Operations**: < 50ms per save/load cycle

### Security Features
- **Password Hashing**: SHA-256 with salt
- **Registry Security**: Proper access permissions
- **Input Validation**: Bounds checking and sanitization
- **Emergency Recovery**: Multiple failsafe mechanisms


## 🐛 Troubleshooting

### Common Issues

**Input Not Locking**
- Check if application is running (tray icon visible)
- Verify hotkey settings in configuration
- Ensure no conflicting applications

**Hotkeys Not Working**
- Check for hotkey conflicts with other applications
- Verify hotkey format in settings
- Restart application after hotkey changes

**Settings Not Saving**
- Check registry permissions
- Verify application has write access to HKCU
- Try running as administrator

**Notifications Not Showing**
- Check notification style settings
- Verify Windows notification permissions
- Try switching to custom notifications

### Recovery Procedures

**Emergency Unlock**
1. Press `ESC` three times rapidly (within 3 seconds)
2. Use Task Manager to end `UtilityApp.exe`
3. Delete registry key: `HKCU\SOFTWARE\UtilityApp`

**Settings Reset**
1. Open Settings Dialog → Data Management
2. Click "Reset to Defaults"
3. Restart application

## 🎨 Attribution

- **Icon**: Courtesy of [Freepik](https://freepik.com)
- **Sound Effects**: Courtesy of [pixabay](https://pixabay.com)
- **Architecture**: Custom Win32 API implementation

## ⚠️ Important Notes

### Usage Guidelines
- **Test Thoroughly**: Always test unlock mechanisms before  use
- **Backup Settings**: Use export feature to backup configurations
- **Monitor Resources**: Application uses minimal system resources
- **Security First**: Change default passwords and hotkeys

### System Impact
- **Input Blocking**: Temporarily disables all keyboard/mouse input
- **Registry Access**: Creates settings under `HKCU\SOFTWARE\UtilityApp`
- **System Hooks**: Installs low-level keyboard/mouse hooks
- **Startup Integration**: Optional Run key registration

### Compatibility
- **Windows 10/11**: Fully supported
- **Administrator Rights**: Not required (registry access via HKCU)
- **Multiple Monitors**: Overlay effects work across all displays
- **High DPI**: Compatible with scaling settings

---

**UtilityApp** - Empowering Windows productivity with advanced input control and privacy features.
