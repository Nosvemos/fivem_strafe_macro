#include "input_sender.h"

void InputSender::pressKey(WORD scanCode)
{
    INPUT input  = {};
    input.type   = INPUT_KEYBOARD;
    input.ki.wScan   = scanCode;
    input.ki.dwFlags = KEYEVENTF_SCANCODE;
    SendInput(1, &input, sizeof(INPUT));
}

void InputSender::releaseKey(WORD scanCode)
{
    INPUT input  = {};
    input.type   = INPUT_KEYBOARD;
    input.ki.wScan   = scanCode;
    input.ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;
    SendInput(1, &input, sizeof(INPUT));
}
