#include "config.h"
#include "macro_engine.h"
#include "hotkey_manager.h"
#include "gui.h"
#include "process_monitor.h"

#include <windows.h>
#include <commctrl.h>

#ifdef _MSC_VER
#pragma comment(linker, "/manifestdependency:\"type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' \
language='*'\"")
#endif




int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE,
                   LPSTR, int)
{
    INITCOMMONCONTROLSEX icc = {};
    icc.dwSize = sizeof(icc);
    icc.dwICC  = ICC_BAR_CLASSES | ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&icc);

    HMODULE shcore = LoadLibraryW(L"shcore.dll");
    if (shcore) {
        auto fn = reinterpret_cast<HRESULT(WINAPI*)(int)>(
            GetProcAddress(shcore, "SetProcessDpiAwareness"));
        if (fn) fn(2);
        FreeLibrary(shcore);
    }

    ConfigManager config;
    MacroEngine   engine(config);
    HotkeyManager hotkeyMgr(config);
    GUI           gui(config, engine, hotkeyMgr);

    if (!gui.create(hInstance)) {
        MessageBoxW(nullptr, L"Failed to create application window.",
                    L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    HWND guiHwnd = gui.getHwnd();

    hotkeyMgr.start([&engine, guiHwnd](bool activate) {
        if (activate)
            engine.start();
        else
            engine.stop();
        PostMessageW(guiHwnd, WM_MACRO_STATE,
                     activate ? 1 : 0, 0);
    });

    SetTimer(guiHwnd, 1, 2000, nullptr);

    int result = gui.messageLoop();

    hotkeyMgr.stop();
    engine.stop();

    return result;
}
