// src/failsafe.h

#pragma once
#include <windows.h>

class Failsafe {
public:
    Failsafe();
    // Call this every time the ESC key is pressed.
    // Returns true if the failsafe condition is met.
    bool recordEscPress();

private:
    int esc_press_count;
    DWORD last_esc_press_time;
};