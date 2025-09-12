// src/failsafe.h

#pragma once
#include <windows.h>

class Failsafe {
public:
    Failsafe();
    // Call this every time the ESC key is pressed.
    // Returns true immediately to trigger failsafe
    bool recordEscPress();

private:
    // No state needed for simplified implementation
};