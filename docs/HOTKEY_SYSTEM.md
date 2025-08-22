# Enhanced Hotkey System Documentation

## 🎯 **Real-time Hotkey Capture System**

The enhanced settings system now includes a sophisticated real-time hotkey capture mechanism that provides intuitive user experience for hotkey configuration.

## 🔧 **Core Features Implemented**

### **1. Real-time Key Combination Display**
- **Progressive Building**: As user presses keys, display updates in real-time
  - User presses `Ctrl` → Display shows: `"Ctrl+"`
  - User adds `Alt` → Display shows: `"Ctrl+Alt+"`
  - User adds `I` → Capture completes with: `"Ctrl+Alt+I"`

### **2. Smart Reset Logic**
- **Fresh Start**: When user begins new combination, previous input is cleared
- **Clean Display**: Final result shows clean combination without trailing "+"
- **Modifier Detection**: System tracks Ctrl, Shift, Alt, Win keys independently

### **3. Enhanced User Interface**
- **Green Hint Text**: "Press key combination..." appears below textbox during capture
- **Single-key Warning**: Red warning text above Apply button for unsafe shortcuts
- **Visual Feedback**: Clear indication when capture mode is active

### **4. Smart Save Logic**
- **Apply-only Saves**: Only Apply button commits changes to settings
- **OK/Cancel = Discard**: With confirmation prompt if changes exist
- **Intelligent Change Detection**: Reverting changes removes "pending" status

## 🎮 **User Experience Flow**

### **Starting Hotkey Capture**
1. User clicks in hotkey textbox
2. Textbox clears immediately  
3. Green hint text appears: "Press key combination..."
4. System begins capturing key presses

### **During Capture**
1. **Modifier Press**: `Ctrl` → Display: `"Ctrl+"`
2. **Additional Modifier**: `+Shift` → Display: `"Ctrl+Shift+"`
3. **Final Key**: `+I` → Capture completes → Display: `"Ctrl+Shift+I"`
4. System automatically ends capture when non-modifier key is pressed

### **Validation & Warnings**
- **Single Key Detection**: If user just presses `"I"` without modifiers
- **Warning Display**: Red text appears above Apply button
- **Message**: "Warning: Single-key shortcuts are not recommended!"

## 💾 **Save Behavior Logic**

### **Button Functions**
- **Apply**: Saves all changes immediately, updates baseline settings
- **OK**: Discards changes (prompts if unsaved changes exist)
- **Cancel**: Discards changes (prompts if unsaved changes exist)

### **Smart Change Detection**
The system tracks what settings have actually changed:
```cpp
bool HasPendingChanges() {
    return (tempSettings != originalSettings);
}
```

### **Tab Switching Logic**
When switching tabs with unsaved changes:
1. **Prompt**: "You have unsaved changes. Do you want to save them?"
2. **Yes**: Save changes and switch tabs
3. **No**: Discard changes and switch tabs  
4. **Cancel**: Stay on current tab

### **Revert Detection**
- If user changes setting then changes it back → No pending changes
- System compares against original baseline, not just "has been modified"

## 🔧 **Technical Implementation**

### **Hotkey Capture Hook**
- **Low-level Keyboard Hook**: Captures all keyboard input during editing
- **Modifier Tracking**: Independent tracking of Ctrl, Shift, Alt, Win states
- **Key Translation**: Converts virtual key codes to readable names

### **Supported Key Types**
- **Letters**: A-Z
- **Numbers**: 0-9  
- **Function Keys**: F1-F12
- **Special Keys**: Esc, Space, Enter, Tab
- **Modifiers**: Ctrl, Shift, Alt, Win (including L/R variants)

### **Key Combination Examples**
```
Ctrl+Shift+I     → Standard combination
Alt+F4           → Function key combination  
Win+L            → Windows key combination
Ctrl+Alt+Delete  → Three-modifier combination
F12              → Single key (with warning)
```

## 🎨 **Visual Feedback System**

### **Text Color Coding**
- **Green**: Hint text during capture (future enhancement: actual green color)
- **Red**: Warning text for unsafe shortcuts (future enhancement: actual red color)
- **Black**: Normal text display

### **UI State Management**
- **Capture Mode**: Textbox cleared, hint visible, hook active
- **Normal Mode**: Textbox shows current hotkey, no special UI elements
- **Warning Mode**: Red warning text visible above Apply button

## 🔍 **Validation Rules**

### **Single-key Warning**
```cpp
bool isSingleKey = (hotkey.find('+') == std::string::npos) && hotkey.length() == 1;
```

### **Recommended Patterns**
- ✅ **Good**: `Ctrl+Shift+Letter`, `Alt+Function`, `Win+Letter`
- ⚠️ **Warning**: `Letter`, `Number`, `Function` (single keys)
- ❌ **Avoid**: System keys like `Ctrl+Alt+Del`

## 🚀 **Usage Instructions**

### **Changing a Hotkey**
1. Open Settings → Lock & Input tab
2. Click in the "Lock Hotkey" textbox
3. Press desired key combination (e.g., Ctrl+Shift+L)
4. System captures and displays the combination
5. Click Apply to save changes

### **Keyboard Capture Behavior**
- **Real-time Updates**: See keys as you press them
- **Automatic Completion**: Non-modifier key ends capture
- **Clean Display**: Final result shows proper format
- **Escape Handling**: ESC during capture cancels operation

## 🔧 **Advanced Features**

### **Hook Management**
- **Install on Capture**: Hook only active during hotkey editing
- **Automatic Cleanup**: Hook removed when capture ends
- **System Integration**: Works with existing input blocker hooks

### **Settings Persistence**
- **File-based Storage**: Ready for configuration file implementation
- **Memory Management**: Efficient temporary settings handling
- **State Tracking**: Smart baseline comparison for change detection

This enhanced system provides a professional, intuitive hotkey configuration experience that matches modern application standards while maintaining system stability and user-friendly feedback.
