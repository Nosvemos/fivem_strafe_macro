#include "config.h"
#include "macro_engine.h"
#include "hotkey_manager.h"
#include "gui.h"

#include <windows.h>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE,
                   LPSTR, int)
{
    // Set high-DPI awareness
    HMODULE shcore = LoadLibraryW(L"shcore.dll");
    if (shcore) {
        auto fn = reinterpret_cast<HRESULT(WINAPI*)(int)>(
            GetProcAddress(shcore, "SetProcessDpiAwareness"));
        if (fn) fn(2); // PROCESS_PER_MONITOR_DPI_AWARE
        FreeLibrary(shcore);
    }

    ConfigManager config;
    MacroEngine   engine(config);
    HotkeyManager hotkeyMgr(config);
    GUI           gui(config, engine, hotkeyMgr);

    if (!gui.init(hInstance)) {
        MessageBoxW(nullptr, L"Failed to initialize modern interface.",
                    L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    HWND guiHwnd = gui.getHwnd();

    // Start hotkey manager
    hotkeyMgr.start([&engine, guiHwnd](bool activate) {
        if (activate)
            engine.start();
        else
            engine.stop();
        PostMessageW(guiHwnd, WM_MACRO_STATE, activate ? 1 : 0, 0);
    });

    // Start timer for FiveM process monitoring (every 2 seconds)
    SetTimer(guiHwnd, 1, 2000, nullptr);

    // Enter message / render loop (blocks until window close)
    gui.run();

    // Cleanup
    hotkeyMgr.stop();
    engine.stop();
    gui.shutdown();

    return 0;
}
