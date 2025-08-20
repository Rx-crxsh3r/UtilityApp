// src/failsafe.cpp

#include "failsafe.h"

// Failsafe configuration
const int FAILSAFE_KEY_COUNT = 3;
const DWORD FAILSAFE_TIME_WINDOW_MS = 3000; // 3 seconds

Failsafe::Failsafe() : esc_press_count(0), last_esc_press_time(0) {}

bool Failsafe::recordEscPress() {
    DWORD current_time = GetTickCount();

    // If the time since the last press is outside the window, reset the count.
    if (current_time - last_esc_press_time > FAILSAFE_TIME_WINDOW_MS) {
        esc_press_count = 1;
    } else {
        esc_press_count++;
    }

    last_esc_press_time = current_time;

    // Check if the failsafe condition is met
    if (esc_press_count >= FAILSAFE_KEY_COUNT) {
        esc_press_count = 0; // Reset after triggering
        return true;
    }

    return false;
}