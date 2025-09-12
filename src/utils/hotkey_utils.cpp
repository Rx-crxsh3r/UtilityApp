// src/utils/hotkey_utils.cpp
// Hotkey parsing and utility functions implementation

#include "hotkey_utils.h"

// Utility function to parse hotkey strings (e.g., "Ctrl+Alt+F12")
bool ParseHotkeyString(const std::string& hotkeyStr, UINT& modifiers, UINT& virtualKey) {
    modifiers = 0;
    virtualKey = 0;

    if (hotkeyStr.empty()) return false;

    // Use const reference to avoid copy
    const std::string& str = hotkeyStr;
    size_t len = str.length();

    // Parse modifiers using more efficient string search
    size_t pos = 0;
    while (pos < len) {
        if (str[pos] == 'C' && pos + 3 < len &&
            str[pos+1] == 't' && str[pos+2] == 'r' && str[pos+3] == 'l') {
            modifiers |= MOD_CONTROL;
            pos += 4;
            if (pos < len && str[pos] == '+') pos++;
        } else if (str[pos] == 'A' && pos + 2 < len &&
                   str[pos+1] == 'l' && str[pos+2] == 't') {
            modifiers |= MOD_ALT;
            pos += 3;
            if (pos < len && str[pos] == '+') pos++;
        } else if (str[pos] == 'S' && pos + 4 < len &&
                   str[pos+1] == 'h' && str[pos+2] == 'i' && str[pos+3] == 'f' && str[pos+4] == 't') {
            modifiers |= MOD_SHIFT;
            pos += 5;
            if (pos < len && str[pos] == '+') pos++;
        } else if (str[pos] == 'W' && pos + 2 < len &&
                   str[pos+1] == 'i' && str[pos+2] == 'n') {
            modifiers |= MOD_WIN;
            pos += 3;
            if (pos < len && str[pos] == '+') pos++;
        } else {
            break; // Found the main key
        }
    }

    // Parse the main key from the remaining string
    if (pos >= len) return false;

    std::string keyStr = str.substr(pos);

    // Optimize key parsing with direct character checks
    if (keyStr.length() == 1) {
        char ch = keyStr[0];
        if ((ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9')) {
            virtualKey = ch;
            return true;
        }
    } else if (keyStr[0] == 'F' && keyStr.length() >= 2 && keyStr.length() <= 3) {
        // Function key (F1-F12)
        int funcNum = 0;
        if (keyStr.length() == 2) {
            funcNum = keyStr[1] - '0';
        } else {
            funcNum = (keyStr[1] - '0') * 10 + (keyStr[2] - '0');
        }
        if (funcNum >= 1 && funcNum <= 12) {
            virtualKey = VK_F1 + (funcNum - 1);
            return true;
        }
    }

    // Special keys - use a more efficient lookup
    static const struct {
        const char* name;
        UINT vk;
    } specialKeys[] = {
        {"ESC", VK_ESCAPE}, {"ESCAPE", VK_ESCAPE},
        {"SPACE", VK_SPACE}, {"ENTER", VK_RETURN}, {"RETURN", VK_RETURN},
        {"TAB", VK_TAB}, {"BACKSPACE", VK_BACK}, {"DELETE", VK_DELETE},
        {"DEL", VK_DELETE}, {"INSERT", VK_INSERT}, {"INS", VK_INSERT},
        {"HOME", VK_HOME}, {"END", VK_END},
        {"PAGEUP", VK_PRIOR}, {"PAGEDOWN", VK_NEXT},
        {"PGUP", VK_PRIOR}, {"PGDN", VK_NEXT}
    };

    for (const auto& key : specialKeys) {
        if (keyStr == key.name) {
            virtualKey = key.vk;
            return true;
        }
    }

    return false;
}
