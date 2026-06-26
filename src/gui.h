#pragma once
#ifndef GUI_H
#define GUI_H

#include "config.h"
#include "macro_engine.h"
#include "hotkey_manager.h"
#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>
#include <string>

// ─── Custom window messages ─────────────────────────────────────────
#define WM_TRAYICON     (WM_USER + 1)
#define WM_MACRO_STATE  (WM_USER + 2)

// ─── Win32 native dark-themed GUI ───────────────────────────────────

class GUI {
public:
    GUI(ConfigManager& config, MacroEngine& engine, HotkeyManager& hotkeyMgr);
    ~GUI();

    bool create(HINSTANCE hInstance);
    int  messageLoop();
    void updateStatus(bool active);
    HWND getHwnd() const { return hwnd_; }

private:
    // ── Window procedure ────────────────────────────────────────────
    static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
    LRESULT handleMessage(HWND, UINT, WPARAM, LPARAM);

    // ── UI helpers ──────────────────────────────────────────────────
    void createControls(HWND hwnd);
    void onCommand(HWND hwnd, WPARAM wParam, LPARAM lParam);
    void onHScroll(HWND hwnd, WPARAM wParam, LPARAM lParam);

    void applyConfigFromUI();
    void loadConfigToUI();
    void updateRandomLabel();
    std::wstring getKeyName(int vk);

    void setupTrayIcon(HWND hwnd);
    void removeTrayIcon();

    // ── Core references ─────────────────────────────────────────────
    ConfigManager& config_;
    MacroEngine&   engine_;
    HotkeyManager& hotkeyMgr_;

    HWND       hwnd_      = nullptr;
    HINSTANCE  hInstance_  = nullptr;
    std::wstring className_;

    bool macroActive_   = false;
    bool ignoreChanges_ = true;   // suppress feedback during loadConfigToUI and creation
    bool isBinding_     = false;

    // ── Child control handles ───────────────────────────────────────
    HWND hBaseDelay_    = nullptr;
    HWND hMinDelay_     = nullptr;
    HWND hMaxDelay_     = nullptr;
    HWND hSliderRandom_ = nullptr;
    HWND hLabelRandom_  = nullptr;
    HWND hBtnTriggerBind_ = nullptr;
    HWND hRadioHold_    = nullptr;
    HWND hRadioToggle_  = nullptr;
    HWND hComboPattern_ = nullptr;
    HWND hCheckFatigue_ = nullptr;
    HWND hEditTitle_    = nullptr;
    HWND hCheckOnTop_   = nullptr;
    HWND hCheckTray_    = nullptr;
    HWND hCheckAutoSprint_ = nullptr;
    HWND hBtnStart_     = nullptr;
    HWND hBtnStop_      = nullptr;
    HWND hLabelStatus_  = nullptr;
    HWND hLabelFiveM_   = nullptr;

    // ── GDI resources ───────────────────────────────────────────────
    HBRUSH hBgBrush_   = nullptr;
    HBRUSH hEditBrush_ = nullptr;
    HFONT  hFont_      = nullptr;
    HFONT  hFontBold_  = nullptr;
    NOTIFYICONDATAW nid_ = {};
    bool   trayAdded_  = false;
};

#endif // GUI_H
