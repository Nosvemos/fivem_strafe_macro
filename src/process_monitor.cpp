#include "process_monitor.h"
#include <tlhelp32.h>
#include <cwctype>

bool ProcessMonitor::matchesFiveMPattern(const wchar_t* exeName)
{
    // Match GTA5.exe exactly (case-insensitive) - dynamically constructed
    wchar_t gta[9];
    gta[0] = L'G'; gta[1] = L'T'; gta[2] = L'A'; gta[3] = L'5';
    gta[4] = L'.'; gta[5] = L'e'; gta[6] = L'x'; gta[7] = L'e';
    gta[8] = L'\0';

    if (_wcsicmp(exeName, gta) == 0) return true;

    // Match any executable containing "fivem" (case-insensitive) - dynamically constructed
    wchar_t fm[6];
    fm[0] = L'f'; fm[1] = L'i'; fm[2] = L'v'; fm[3] = L'e';
    fm[4] = L'm'; fm[5] = L'\0';

    std::wstring name(exeName);
    for (auto& c : name) {
        c = static_cast<wchar_t>(towlower(c));
    }
    return name.find(fm) != std::wstring::npos;
}

bool ProcessMonitor::isFiveMRunning()
{
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return false;

    PROCESSENTRY32W pe = {};
    pe.dwSize = sizeof(pe);

    bool found = false;
    if (Process32FirstW(snap, &pe)) {
        do {
            if (matchesFiveMPattern(pe.szExeFile)) {
                found = true;
                break;
            }
        } while (Process32NextW(snap, &pe));
    }

    CloseHandle(snap);
    return found;
}

bool ProcessMonitor::isFiveMForeground()
{
    HWND fg = GetForegroundWindow();
    if (!fg) return false;

    DWORD pid;
    GetWindowThreadProcessId(fg, &pid);

    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return false;

    PROCESSENTRY32W pe = {};
    pe.dwSize = sizeof(pe);

    bool match = false;
    if (Process32FirstW(snap, &pe)) {
        do {
            if (pe.th32ProcessID == pid) {
                if (matchesFiveMPattern(pe.szExeFile)) {
                    match = true;
                    break;
                }
            }
        } while (!match && Process32NextW(snap, &pe));
    }

    CloseHandle(snap);
    return match;
}

std::wstring ProcessMonitor::getGameWindowTitle()
{
    HWND fg = GetForegroundWindow();
    if (!fg) return L"";

    wchar_t buf[256];
    GetWindowTextW(fg, buf, 256);
    return std::wstring(buf);
}
