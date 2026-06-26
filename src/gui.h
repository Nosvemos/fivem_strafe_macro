#pragma once
#ifndef GUI_H
#define GUI_H

#include "config.h"
#include "macro_engine.h"
#include "hotkey_manager.h"
#include <windows.h>
#include <d3d11.h>
#include <string>

// ─── Custom window messages ─────────────────────────────────────────
#define WM_TRAYICON     (WM_USER + 1)
#define WM_MACRO_STATE  (WM_USER + 2)

// ─── DirectX 11 & Dear ImGui GUI ────────────────────────────────────

class GUI {
public:
    GUI(ConfigManager& config, MacroEngine& engine, HotkeyManager& hotkeyMgr);
    ~GUI();

    /// Initialize the Win32 window, DirectX 11, and Dear ImGui.
    bool init(HINSTANCE hInstance);

    /// Main message loop and ImGui render loop.
    void run();

    /// Clean up ImGui and DX11 resources.
    void shutdown();

    /// Update status of the macro (active/idle).
    void updateStatus(bool active);

    HWND getHwnd() const { return hwnd_; }

private:
    // ── Window procedure ────────────────────────────────────────────
    static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
    LRESULT handleMessage(HWND, UINT, WPARAM, LPARAM);

    // ── DirectX 11 helpers ──────────────────────────────────────────
    bool createDeviceD3D(HWND hWnd);
    void cleanupDeviceD3D();
    void createRenderTarget();
    void cleanupRenderTarget();

    // ── ImGui render methods ────────────────────────────────────────
    void renderFrame();
    void setupTheme();

    // ── Tray icon management ────────────────────────────────────────
    void setupTrayIcon();
    void removeTrayIcon();

    std::wstring getKeyName(int vk);

    // ── Core references ─────────────────────────────────────────────
    ConfigManager& config_;
    MacroEngine&   engine_;
    HotkeyManager& hotkeyMgr_;

    HWND       hwnd_       = nullptr;
    HINSTANCE  hInstance_  = nullptr;
    std::wstring className_;

    bool macroActive_   = false;
    bool isBinding_     = false;
    bool trayAdded_     = false;
    NOTIFYICONDATAW nid_ = {};

    // ── DirectX 11 interfaces ───────────────────────────────────────
    ID3D11Device*           pd3dDevice_           = nullptr;
    ID3D11DeviceContext*    pd3dDeviceContext_    = nullptr;
    IDXGISwapChain*         pSwapChain_           = nullptr;
    ID3D11RenderTargetView* mainRenderTargetView_ = nullptr;
};

#endif // GUI_H
