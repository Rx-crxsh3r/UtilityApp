// src/failsafe.cpp

#include "failsafe.h"

Failsafe::Failsafe() {}

bool Failsafe::recordEscPress() {
    // Simplified: Trigger immediately on any ESC press
    // This is sufficient for emergency access without complex timing
    return true;
}