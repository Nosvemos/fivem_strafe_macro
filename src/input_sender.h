#pragma once
#ifndef INPUT_SENDER_H
#define INPUT_SENDER_H

#include <windows.h>

// ─── Low-level key sender using hardware scan codes ─────────────────
//
//  Uses KEYEVENTF_SCANCODE flag so the OS treats the input as if it
//  came from a physical keyboard scan rather than a virtual key.
// ────────────────────────────────────────────────────────────────────

class InputSender {
public:
    // Hardware scan codes (Set 1 — standard US layout)
    static constexpr WORD SCAN_A = 0x1E;
    static constexpr WORD SCAN_D = 0x20;
    static constexpr WORD SCAN_W = 0x11;
    static constexpr WORD SCAN_S = 0x1F;
    static constexpr WORD SCAN_SHIFT = 0x2A;

    /// Simulate a key press (key down) for the given scan code.
    static void pressKey(WORD scanCode);

    /// Simulate a key release (key up) for the given scan code.
    static void releaseKey(WORD scanCode);
};

#endif // INPUT_SENDER_H
