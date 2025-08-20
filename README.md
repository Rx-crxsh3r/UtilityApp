# UtilityApp

A lightweight, high-performance Windows utility application written in C++ using the Win32 API that runs silently in the background and provides global input locking capabilities.

## 🎯 Features

### Core Functionality
- **Background Operation**: Runs silently with minimal CPU and RAM footprint
- **System Tray Integration**: Clean interface via notification area icon
- **Global Input Locking**: Lock/unlock keyboard and mouse system-wide
- **Multiple Unlock Methods**: Hotkey combinations or password sequence
- **Failsafe Mechanism**: Emergency exit using ESC key sequence
- **Windows Notifications**: Toast notifications for all major actions

### User Interface
- **Enhanced Context Menu**: Right-click tray icon for full options
- **Double-click Action**: Quick input toggle via tray icon
- **Visual Feedback**: Status indicators and notifications

## 🔑 Hotkeys & Controls

| Action | Method 1 | Method 2 |
|--------|----------|----------|
| **Lock Input** | `Ctrl + Shift + I` | Right-click → Lock/Unlock |
| **Unlock Input** | `Ctrl + O` | Type password: `10203040` |
| **Emergency Exit** | Press `ESC` 3x within 3 seconds | Right-click → Exit |
| **Quick Toggle** | Double-click tray icon | - |

## 🚀 Getting Started

### Prerequisites
- Windows 10/11
- MinGW-w64 (GCC compiler)
- Windows SDK

### Building from Source

1. **Clone the repository**
   ```bash
   git clone https://github.com/Rx-crxsh3r/UtilityApp
   cd UtilityApp
   ```

2. **Compile resources**
   ```bash
   cd resources
   windres resources.rc -o resources.o
   cd ../src
   ```

3. **Compile source files**
   ```bash
   g++ -c main.cpp tray_icon.cpp input_blocker.cpp failsafe.cpp notifications.cpp
   ```

4. **Link and create executable**
   ```bash
   g++ main.o tray_icon.o input_blocker.o failsafe.o notifications.o ../resources/resources.o -mwindows -luser32 -lkernel32 -lshell32 -lgdi32 -o utilityapp.exe
   ```

### Quick Build 
_**You can alternatively use the "build.bat" to create the app or the "clean.bat" to remove the existing build files.**_

## 📁 Project Structure

```
UtilityApp/
├── src/
│   ├── main.cpp              # Entry point & window management
│   ├── tray_icon.cpp/.h      # System tray integration
│   ├── input_blocker.cpp/.h  # Keyboard/mouse locking
│   ├── failsafe.cpp/.h       # Emergency exit mechanism
│   ├── notifications.cpp/.h  # Windows notifications
│   └── resource.h            # Resource definitions
├── resources/
│   ├── icon.ico              # Application icon
│   ├── resources.rc          # Resource script
│   └── resources.o           # Compiled resources
├── .gitignore
├── README.md
└── utilityapp.exe            # Compiled executable
```

## 🛠️ Technical Details

### Technologies Used
- **Language**: C++17
- **API**: Win32 API (Pure Windows API, no frameworks)
- **Compiler**: GCC (MinGW-w64)
- **Architecture**: x64

### Key Components

| Component | Purpose |
|-----------|---------|
| `main.cpp` | Application entry point, message loop, window management |
| `tray_icon.*` | System tray icon creation, context menu handling |
| `input_blocker.*` | Low-level keyboard/mouse hooks, input blocking logic |
| `failsafe.*` | Emergency exit mechanism with timing logic |
| `notifications.*` | Windows toast notifications and balloon tips |

### Performance 
- **Memory Usage**: < 5MB RAM when idle
- **CPU Usage**: ~0% when idle, minimal during input processing
- **Startup Time**: < 100ms
- **Dependencies**: None (uses only Windows system libraries)

## 🔧 Configuration

### Current Default Settings
- **Lock Hotkey**: `Ctrl + Shift + I`
- **Unlock Hotkey**: `Ctrl + O`
- **Unlock Password**: `10203040`
- **Failsafe Trigger**: 3 ESC presses within 3 seconds


## 🎨 Attribution

- Icon image courtesy of [Freepik](https://www.freepik.com)


## ⚠️ Disclaimer

This utility modifies system-level input handling. Use responsibly and ensure you understand the unlock mechanisms before deploying in production environments.