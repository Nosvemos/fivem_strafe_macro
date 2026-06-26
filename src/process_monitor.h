#pragma once
#ifndef PROCESS_MONITOR_H
#define PROCESS_MONITOR_H

#include <windows.h>
#include <string>

class ProcessMonitor {
public:
    static bool isFiveMRunning();
    static bool isFiveMForeground();
    static std::wstring getGameWindowTitle();

private:
    static bool matchesFiveMPattern(const wchar_t* exeName);
};

#endif
