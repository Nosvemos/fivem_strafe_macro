#include "gui.h"
#include "process_monitor.h"
#include <dwmapi.h>
#include <random>
#include <string>
#include <cstdlib>
#include <algorithm>

// ════════════════════════════════════════════════════════════════════
//  CONSTANTS
// ════════════════════════════════════════════════════════════════════

// ── Control IDs ─────────────────────────────────────────────────────
#define IDC_EDIT_BASE_DELAY   101
#define IDC_EDIT_MIN_DELAY    102
#define IDC_EDIT_MAX_DELAY    103
#define IDC_SLIDER_RANDOM     104
#define IDC_LABEL_RANDOM      105
#define IDC_BTN_TRIGGER_BIND  106
#define IDC_RADIO_HOLD        107
#define IDC_RADIO_TOGGLE      108
#define IDC_COMBO_PATTERN     109
#define IDC_CHECK_FATIGUE     110
#define IDC_EDIT_TITLE        111
#define IDC_CHECK_ONTOP       112
#define IDC_CHECK_TRAY        113
#define IDC_BTN_START         114
#define IDC_BTN_STOP          115
#define IDC_LABEL_STATUS      116
#define IDC_LABEL_FIVEM       117
#define IDC_CHECK_GAMEONLY    118
#define IDC_CHECK_AUTOSPRINT  119

#define IDC_SECTION_TIMING    200
#define IDC_SECTION_TRIGGER   201
#define IDC_SECTION_PATTERN   202
#define IDC_SECTION_ADVANCED  203

// ── Dark-theme colours (Dracula-inspired) ───────────────────────────
#define CLR_BG            RGB(25, 25, 35)
#define CLR_BG_EDIT       RGB(40, 42, 54)
#define CLR_TEXT          RGB(210, 210, 220)
#define CLR_SECTION       RGB(139, 233, 253)
#define CLR_ACCENT        RGB(80, 250, 123)
#define CLR_ACCENT_RED    RGB(255, 85, 85)
#define CLR_BTN_START_BG  RGB(40, 167, 69)
#define CLR_BTN_STOP_BG   RGB(220, 53, 69)
#define CLR_BTN_TEXT      RGB(255, 255, 255)
#define CLR_SEPARATOR     RGB(60, 63, 80)

std::wstring GUI::getKeyName(int vk)
{
    if (vk == VK_LBUTTON) return L"Left Mouse";
    if (vk == VK_RBUTTON) return L"Right Mouse";
    if (vk == VK_MBUTTON) return L"Middle Mouse";
    if (vk == VK_XBUTTON1) return L"Mouse 4";
    if (vk == VK_XBUTTON2) return L"Mouse 5";
    
    // Map extended keys correctly
    UINT scanCode = MapVirtualKeyW(vk, MAPVK_VK_TO_VSC);
    switch (vk) {
        case VK_LEFT: case VK_UP: case VK_RIGHT: case VK_DOWN:
        case VK_PRIOR: case VK_NEXT: case VK_END: case VK_HOME:
        case VK_INSERT: case VK_DELETE: case VK_DIVIDE: case VK_NUMLOCK:
            scanCode |= 0x100; // set extended bit
            break;
    }
    
    wchar_t name[128] = {0};
    if (GetKeyNameTextW(scanCode << 16, name, 128) > 0) {
        return name;
    }
    return L"Key " + std::to_wstring(vk);
}

// ── Pattern names ───────────────────────────────────────────────────
static const wchar_t* PATTERN_NAMES[] = {
    L"A \x2194 D",
    L"A \x2192 D \x2192 A",
    L"Only A",
    L"Only D",
    L"W \x2192 A \x2192 S \x2192 D",
    L"W \x2192 D \x2192 S \x2192 A",
    L"D \x2192 S \x2192 A \x2192 W",
    L"W \x2192 A \x2192 D \x2192 S",
};
static constexpr int PATTERN_COUNT = 8;

// ════════════════════════════════════════════════════════════════════
//  HELPERS
// ════════════════════════════════════════════════════════════════════

/// Generate a random 12-character class name (anti-detection).
static std::wstring generateClassName()
{
    static const wchar_t chars[] = L"abcdefghijklmnopqrstuvwxyz";
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(0, 25);
    std::wstring name;
    name.reserve(12);
    for (int i = 0; i < 12; ++i) name += chars[dist(gen)];
    return name;
}

// ════════════════════════════════════════════════════════════════════
//  CONSTRUCTION / DESTRUCTION
// ════════════════════════════════════════════════════════════════════

GUI::GUI(ConfigManager& config, MacroEngine& engine, HotkeyManager& hotkeyMgr)
    : config_(config), engine_(engine), hotkeyMgr_(hotkeyMgr)
{
    className_ = generateClassName();
}

GUI::~GUI()
{
    removeTrayIcon();
    if (hBgBrush_)   DeleteObject(hBgBrush_);
    if (hEditBrush_) DeleteObject(hEditBrush_);
    if (hFont_)      DeleteObject(hFont_);
    if (hFontBold_)  DeleteObject(hFontBold_);
}

// ════════════════════════════════════════════════════════════════════
//  WINDOW CREATION
// ════════════════════════════════════════════════════════════════════




bool GUI::create(HINSTANCE hInstance)
{
    
    hInstance_ = hInstance;

    // ── Register window class (random name) ──
    WNDCLASSEXW wc = {};
    wc.cbSize        = sizeof(wc);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = WndProc;
    wc.hInstance      = hInstance;
    wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr;   // we paint ourselves
    wc.lpszClassName = className_.c_str();
    wc.hIcon         = LoadIconW(hInstance, MAKEINTRESOURCEW(1));
    wc.hIconSm       = LoadIconW(hInstance, MAKEINTRESOURCEW(1));

    
    if (!RegisterClassExW(&wc)) {
        
        return false;
    }

    
    hFont_ = CreateFontW(-14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

    hFontBold_ = CreateFontW(-14, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

    // ── Create brushes ──
    hBgBrush_   = CreateSolidBrush(CLR_BG);
    hEditBrush_ = CreateSolidBrush(CLR_BG_EDIT);

    // ── Window style: fixed-size, no maximize, always visible ──
    DWORD style   = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    DWORD exStyle = WS_EX_APPWINDOW;
    RECT  rc      = { 0, 0, 440, 555 };
    AdjustWindowRectEx(&rc, style, FALSE, exStyle);

    auto cfg = config_.getConfig();
    

    hwnd_ = CreateWindowExW(
        exStyle, className_.c_str(), cfg.windowTitle.c_str(), style,
        CW_USEDEFAULT, CW_USEDEFAULT,
        rc.right - rc.left, rc.bottom - rc.top,
        nullptr, nullptr, hInstance, this);

    

    if (!hwnd_) {
        
        return false;
    }

    
    // ── Dark title bar (Windows 10 20H1+ / Windows 11) ──
    BOOL darkMode = TRUE;
    if (FAILED(DwmSetWindowAttribute(hwnd_, 20, &darkMode, sizeof(darkMode)))) {
        DwmSetWindowAttribute(hwnd_, 19, &darkMode, sizeof(darkMode));  // fallback
    }

    if (cfg.alwaysOnTop) {
        SetWindowPos(hwnd_, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    }

    
    ShowWindow(hwnd_, SW_SHOW);
    UpdateWindow(hwnd_);
    
    return true;
}

int GUI::messageLoop()
{
    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return static_cast<int>(msg.wParam);
}

// ════════════════════════════════════════════════════════════════════
//  WINDOW PROCEDURE
// ════════════════════════════════════════════════════════════════════

LRESULT CALLBACK GUI::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    GUI* self;

    if (msg == WM_NCCREATE) {
        auto cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
        self = static_cast<GUI*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    } else {
        self = reinterpret_cast<GUI*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }

    if (self) return self->handleMessage(hwnd, msg, wParam, lParam);
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT GUI::handleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {

    // ── Initialisation ──────────────────────────────────────────────
    case WM_CREATE:
        createControls(hwnd);
        loadConfigToUI();
        return 0;

    // ── Commands (buttons, combos, edits, radios, checks) ───────────
    case WM_COMMAND:
        onCommand(hwnd, wParam, lParam);
        return 0;

    // ── Slider change ───────────────────────────────────────────────
    case WM_HSCROLL:
        onHScroll(hwnd, wParam, lParam);
        return 0;

    // ── Dark theme: static controls (labels, radios, checkboxes) ────
    case WM_CTLCOLORSTATIC: {
        
        HDC  hdc  = reinterpret_cast<HDC>(wParam);
        HWND hCtl = reinterpret_cast<HWND>(lParam);
        SetBkColor(hdc, CLR_BG);

        int id = GetDlgCtrlID(hCtl);
        if (id == IDC_LABEL_STATUS)
            SetTextColor(hdc, macroActive_ ? CLR_ACCENT : CLR_ACCENT_RED);
        else if (id >= IDC_SECTION_TIMING && id <= IDC_SECTION_ADVANCED)
            SetTextColor(hdc, CLR_SECTION);
        else
            SetTextColor(hdc, CLR_TEXT);

        
        return reinterpret_cast<LRESULT>(hBgBrush_);
    }

    // ── Dark theme: edit boxes ──────────────────────────────────────
    case WM_CTLCOLOREDIT: {
        
        HDC hdc = reinterpret_cast<HDC>(wParam);
        SetTextColor(hdc, CLR_TEXT);
        SetBkColor(hdc, CLR_BG_EDIT);
        
        return reinterpret_cast<LRESULT>(hEditBrush_);
    }

    // ── Dark theme: combo-box dropdown list ─────────────────────────
    case WM_CTLCOLORLISTBOX: {
        
        HDC hdc = reinterpret_cast<HDC>(wParam);
        SetTextColor(hdc, CLR_TEXT);
        SetBkColor(hdc, CLR_BG_EDIT);
        
        return reinterpret_cast<LRESULT>(hEditBrush_);
    }

    // ── Owner-drawn buttons (START / STOP / BIND) ───────────────────
    case WM_DRAWITEM: {
        auto* dis = reinterpret_cast<DRAWITEMSTRUCT*>(lParam);
        if (dis->CtlID != IDC_BTN_START && dis->CtlID != IDC_BTN_STOP && dis->CtlID != IDC_BTN_TRIGGER_BIND)
            break;

        bool pressed = (dis->itemState & ODS_SELECTED) != 0;

        // Clear corners and fill background with parent background first to avoid sharp corner artifacts
        HBRUSH hParentBr = CreateSolidBrush(CLR_BG);
        FillRect(dis->hDC, &dis->rcItem, hParentBr);
        DeleteObject(hParentBr);

        if (dis->CtlID == IDC_BTN_TRIGGER_BIND) {
            COLORREF bg = pressed ? RGB(55, 57, 72) : (isBinding_ ? RGB(139, 233, 253) : CLR_BG_EDIT);
            COLORREF textCol = isBinding_ ? CLR_BG : CLR_TEXT;

            HBRUSH br = CreateSolidBrush(bg);
            HBRUSH oldBrush = static_cast<HBRUSH>(SelectObject(dis->hDC, br));

            HPEN pen = CreatePen(PS_SOLID, 1, isBinding_ ? CLR_SECTION : CLR_SEPARATOR);
            HPEN oldPen = static_cast<HPEN>(SelectObject(dis->hDC, pen));

            RoundRect(dis->hDC, dis->rcItem.left, dis->rcItem.top, dis->rcItem.right, dis->rcItem.bottom, 6, 6);

            SelectObject(dis->hDC, oldBrush);
            SelectObject(dis->hDC, oldPen);
            DeleteObject(br);
            DeleteObject(pen);

            SetBkMode(dis->hDC, TRANSPARENT);
            SetTextColor(dis->hDC, textCol);
            SelectObject(dis->hDC, hFont_);

            wchar_t text[128] = {0};
            GetWindowTextW(dis->hwndItem, text, 128);
            DrawTextW(dis->hDC, text, -1, &dis->rcItem, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        } else {
            bool isStart = (dis->CtlID == IDC_BTN_START);
            COLORREF bg  = isStart ? CLR_BTN_START_BG : CLR_BTN_STOP_BG;
            if (pressed) {
                bg = RGB(GetRValue(bg) * 3 / 4,
                         GetGValue(bg) * 3 / 4,
                         GetBValue(bg) * 3 / 4);
            }

            HBRUSH br = CreateSolidBrush(bg);
            HBRUSH oldBrush = static_cast<HBRUSH>(SelectObject(dis->hDC, br));

            HPEN pen = CreatePen(PS_SOLID, 1, CLR_SEPARATOR);
            HPEN oldPen = static_cast<HPEN>(SelectObject(dis->hDC, pen));

            RoundRect(dis->hDC, dis->rcItem.left, dis->rcItem.top, dis->rcItem.right, dis->rcItem.bottom, 8, 8);

            SelectObject(dis->hDC, oldBrush);
            SelectObject(dis->hDC, oldPen);
            DeleteObject(br);
            DeleteObject(pen);

            SetBkMode(dis->hDC, TRANSPARENT);
            SetTextColor(dis->hDC, CLR_BTN_TEXT);
            SelectObject(dis->hDC, hFontBold_);

            const wchar_t* text = isStart ? L"\x25B6  START" : L"\x25A0  STOP";
            DrawTextW(dis->hDC, text, -1, &dis->rcItem, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        }
        return TRUE;
    }

    // ── Background painting ─────────────────────────────────────────
    case WM_ERASEBKGND: {
        HDC hdc = reinterpret_cast<HDC>(wParam);
        RECT rc;
        GetClientRect(hwnd, &rc);
        FillRect(hdc, &rc, hBgBrush_);
        return 1;
    }

    // ── Separator lines ─────────────────────────────────────────────
    case WM_PAINT: {
        
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        HPEN pen    = CreatePen(PS_SOLID, 1, CLR_SEPARATOR);
        HPEN oldPen = static_cast<HPEN>(SelectObject(hdc, pen));

        static const int sepY[] = { 35, 195, 295, 395, 470 };
        for (int y : sepY) {
            MoveToEx(hdc, 15, y, nullptr);
            LineTo(hdc, 425, y);
        }

        SelectObject(hdc, oldPen);
        DeleteObject(pen);
        EndPaint(hwnd, &ps);
        
        return 0;
    }

    // ── Macro state change (posted from hotkey callback) ────────────
    case WM_MACRO_STATE:
        updateStatus(wParam != 0);
        return 0;

    // ── Tray icon interactions ──────────────────────────────────────
    case WM_TRAYICON:
        if (lParam == WM_LBUTTONUP || lParam == WM_LBUTTONDBLCLK) {
            ShowWindow(hwnd, SW_RESTORE);
            SetForegroundWindow(hwnd);
        }
        return 0;

    // ── Minimize to tray ────────────────────────────────────────────
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED && config_.getConfig().minimizeToTray) {
            ShowWindow(hwnd, SW_HIDE);
            setupTrayIcon(hwnd);
        }
        return 0;

    case WM_TIMER: {
        if (wParam == 2) {
            if (!isBinding_) {
                KillTimer(hwnd, 2);
                return 0;
            }
            
            for (int i = 1; i < 255; i++) {
                // Ignore LBUTTON to avoid accidental binding on the initial click
                if (i == VK_LBUTTON) continue;
                
                if (GetAsyncKeyState(i) & 0x8000) {
                    isBinding_ = false;
                    KillTimer(hwnd, 2);
                    
                    if (i != VK_ESCAPE) { // Escape cancels binding
                        config_.setTriggerKey(i);
                        SetWindowTextW(hBtnTriggerBind_, getKeyName(i).c_str());
                    } else {
                        SetWindowTextW(hBtnTriggerBind_, getKeyName(config_.getConfig().triggerKey).c_str());
                    }
                    break;
                }
            }
            return 0;
        }

        if (wParam == 1) {
            bool fiveMUp = ProcessMonitor::isFiveMRunning();
            bool fg      = ProcessMonitor::isFiveMForeground();
            SetWindowTextW(hLabelFiveM_,
                fiveMUp
                    ? (fg ? L"\x25CF FiveM: Active"
                          : L"\x25B9 FiveM: Background")
                    : L"\x25CB FiveM: Not running");
            
            InvalidateRect(hLabelFiveM_, nullptr, TRUE);
        }
        return 0;
    }

    // ── Close: clean shutdown ───────────────────────────────────────
    case WM_CLOSE:
        hotkeyMgr_.stop();
        engine_.stop();
        removeTrayIcon();
        DestroyWindow(hwnd);
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

// ════════════════════════════════════════════════════════════════════
//  CONTROL CREATION
// ════════════════════════════════════════════════════════════════════

void GUI::createControls(HWND hwnd)
{
    
    auto make = [&](const wchar_t* cls, const wchar_t* text,
                    DWORD style, int x, int y, int w, int h, int id) -> HWND
    {
        HWND res = CreateWindowW(cls, text, WS_CHILD | WS_VISIBLE | style,
                             x, y, w, h, hwnd,
                             reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
                             hInstance_, nullptr);
        return res;
    };

    

    // ┌─── Section 1: TIMING ────────────────────────────────────────┐
    make(L"STATIC", L"\x26A1 TIMING",  SS_LEFT, 15, 10, 200, 20, IDC_SECTION_TIMING);

    make(L"STATIC", L"Base Delay (ms):", SS_LEFT, 25, 48, 155, 20, 0);
    hBaseDelay_ = make(L"EDIT", L"50", WS_BORDER | ES_NUMBER | ES_CENTER,
                       200, 45, 80, 24, IDC_EDIT_BASE_DELAY);

    make(L"STATIC", L"Min Delay (ms):",  SS_LEFT, 25, 78, 155, 20, 0);
    hMinDelay_ = make(L"EDIT", L"30", WS_BORDER | ES_NUMBER | ES_CENTER,
                      200, 75, 80, 24, IDC_EDIT_MIN_DELAY);

    make(L"STATIC", L"Max Delay (ms):",  SS_LEFT, 25, 108, 155, 20, 0);
    hMaxDelay_ = make(L"EDIT", L"80", WS_BORDER | ES_NUMBER | ES_CENTER,
                      200, 105, 80, 24, IDC_EDIT_MAX_DELAY);

    
    make(L"STATIC", L"Randomization:",   SS_LEFT, 25, 141, 155, 20, 0);
    hSliderRandom_ = make(TRACKBAR_CLASSW, L"", TBS_HORZ | TBS_NOTICKS,
                          200, 138, 150, 25, IDC_SLIDER_RANDOM);
    SendMessageW(hSliderRandom_, TBM_SETRANGE, TRUE, MAKELPARAM(0, 100));
    SendMessageW(hSliderRandom_, TBM_SETPOS,   TRUE, 20);
    hLabelRandom_ = make(L"STATIC", L"20%", SS_LEFT, 360, 141, 50, 20, IDC_LABEL_RANDOM);

    
    make(L"STATIC", L"\x25C9 TRIGGER", SS_LEFT, 15, 170, 200, 20, IDC_SECTION_TRIGGER);

    make(L"STATIC", L"Trigger Key:",   SS_LEFT, 25, 210, 155, 20, 0);
    hBtnTriggerBind_ = make(L"BUTTON", L"Click to bind...", BS_OWNERDRAW,
                            200, 207, 170, 24, IDC_BTN_TRIGGER_BIND);

    make(L"STATIC", L"Mode:",          SS_LEFT, 25, 242, 155, 20, 0);
    hRadioHold_   = make(L"BUTTON", L"Hold",   BS_AUTORADIOBUTTON | WS_GROUP,
                         200, 240, 80, 22, IDC_RADIO_HOLD);
    hRadioToggle_ = make(L"BUTTON", L"Toggle", BS_AUTORADIOBUTTON,
                         295, 240, 80, 22, IDC_RADIO_TOGGLE);
    SendMessageW(hRadioHold_, BM_SETCHECK, BST_CHECKED, 0);

    // ┌─── Section 3: PATTERN ───────────────────────────────────────┐
    make(L"STATIC", L"\x21C4 PATTERN", SS_LEFT, 15, 270, 200, 20, IDC_SECTION_PATTERN);

    make(L"STATIC", L"Pattern:",       SS_LEFT, 25, 310, 155, 20, 0);
    hComboPattern_ = make(L"COMBOBOX", L"", CBS_DROPDOWNLIST | WS_VSCROLL,
                          200, 307, 170, 200, IDC_COMBO_PATTERN);
    for (int i = 0; i < PATTERN_COUNT; ++i)
        SendMessageW(hComboPattern_, CB_ADDSTRING, 0,
                     reinterpret_cast<LPARAM>(PATTERN_NAMES[i]));
    SendMessageW(hComboPattern_, CB_SETCURSEL, 0, 0);

    hCheckFatigue_ = make(L"BUTTON", L"Fatigue Simulation", BS_AUTOCHECKBOX,
                          25, 340, 200, 22, IDC_CHECK_FATIGUE);

    // ┌─── Section 4: ADVANCED ──────────────────────────────────────┐
    make(L"STATIC", L"\x2699 ADVANCED", SS_LEFT, 15, 370, 200, 20, IDC_SECTION_ADVANCED);

    make(L"STATIC", L"Window Title:",  SS_LEFT, 25, 410, 155, 20, 0);
    hEditTitle_ = make(L"EDIT", L"Volume Control", WS_BORDER | ES_AUTOHSCROLL,
                       200, 407, 200, 24, IDC_EDIT_TITLE);

    hCheckOnTop_ = make(L"BUTTON", L"Always On Top",    BS_AUTOCHECKBOX,
                        25, 435, 160, 22, IDC_CHECK_ONTOP);
    SendMessageW(hCheckOnTop_, BM_SETCHECK, BST_CHECKED, 0);

    hCheckTray_  = make(L"BUTTON", L"Minimize to Tray", BS_AUTOCHECKBOX,
                        210, 435, 170, 22, IDC_CHECK_TRAY);
    SendMessageW(hCheckTray_, BM_SETCHECK, BST_CHECKED, 0);

    hCheckAutoSprint_ = make(L"BUTTON", L"Auto Sprint (Shift)", BS_AUTOCHECKBOX,
                             25, 460, 180, 22, IDC_CHECK_AUTOSPRINT);
    SendMessageW(hCheckAutoSprint_, BM_SETCHECK, BST_CHECKED, 0);

    // ┌─── Buttons + Status ─────────────────────────────────────────┐
    hBtnStart_ = make(L"BUTTON", L"START", BS_OWNERDRAW,
                      80, 490, 120, 35, IDC_BTN_START);
    hBtnStop_  = make(L"BUTTON", L"STOP",  BS_OWNERDRAW,
                      240, 490, 120, 35, IDC_BTN_STOP);

    hLabelStatus_ = make(L"STATIC", L"\x25CF Idle", SS_LEFT,
                         15, 530, 200, 20, IDC_LABEL_STATUS);

    hLabelFiveM_ = make(L"STATIC", L"\x25CF FiveM: Checking...", SS_LEFT,
                        220, 530, 190, 20, IDC_LABEL_FIVEM);

    
    // ── Set font on every child control ──
    EnumChildWindows(hwnd, [](HWND child, LPARAM lp) -> BOOL {
        SendMessageW(child, WM_SETFONT, static_cast<WPARAM>(lp), TRUE);
        return TRUE;
    }, reinterpret_cast<LPARAM>(hFont_));
    

    // ── Override section headers with bold ──
    for (int id = IDC_SECTION_TIMING; id <= IDC_SECTION_ADVANCED; ++id) {
        HWND h = GetDlgItem(hwnd, id);
        if (h) SendMessageW(h, WM_SETFONT, reinterpret_cast<WPARAM>(hFontBold_), TRUE);
    }
}

// ════════════════════════════════════════════════════════════════════
//  COMMAND HANDLING
// ════════════════════════════════════════════════════════════════════

void GUI::onCommand(HWND /*hwnd*/, WPARAM wParam, LPARAM /*lParam*/)
{
    int id   = LOWORD(wParam);
    int code = HIWORD(wParam);

    switch (id) {
    case IDC_BTN_START:
        engine_.start();
        updateStatus(true);
        break;

    case IDC_BTN_STOP:
        engine_.stop();
        updateStatus(false);
        break;

    case IDC_EDIT_BASE_DELAY:
    case IDC_EDIT_MIN_DELAY:
    case IDC_EDIT_MAX_DELAY:
    case IDC_EDIT_TITLE:
        if (code == EN_CHANGE) applyConfigFromUI();
        break;

    case IDC_BTN_TRIGGER_BIND:
        if (!isBinding_) {
            isBinding_ = true;
            SetWindowTextW(hBtnTriggerBind_, L"[Press any key...]");
            SetTimer(hwnd_, 2, 10, nullptr);
        }
        break;

    case IDC_COMBO_PATTERN:
        if (code == CBN_SELCHANGE) applyConfigFromUI();
        break;

    case IDC_RADIO_HOLD:
    case IDC_RADIO_TOGGLE:
    case IDC_CHECK_FATIGUE:
        applyConfigFromUI();
        break;

    case IDC_CHECK_ONTOP:
    case IDC_CHECK_TRAY:
    case IDC_CHECK_AUTOSPRINT:
        applyConfigFromUI();
        break;
    }
}

void GUI::onHScroll(HWND /*hwnd*/, WPARAM /*wParam*/, LPARAM lParam)
{
    if (reinterpret_cast<HWND>(lParam) == hSliderRandom_) {
        updateRandomLabel();
        applyConfigFromUI();
    }
}

// ════════════════════════════════════════════════════════════════════
//  CONFIG ↔ UI SYNC
// ════════════════════════════════════════════════════════════════════

void GUI::applyConfigFromUI()
{
    if (ignoreChanges_) return;

    ignoreChanges_ = true; // prevent recursion

    MacroConfig cfg = config_.getConfig(); // Keep existing triggerKey since it's updated async
    wchar_t buf[256] = {0};

    // ── Timing (with validation) ──
    GetWindowTextW(hBaseDelay_, buf, 32);
    cfg.baseDelay = buf[0] ? static_cast<int>(wcstol(buf, nullptr, 10)) : 50;
    cfg.baseDelay = std::clamp(cfg.baseDelay, 1, 1000);

    GetWindowTextW(hMinDelay_, buf, 32);
    cfg.minDelay = buf[0] ? static_cast<int>(wcstol(buf, nullptr, 10)) : 30;
    cfg.minDelay = std::clamp(cfg.minDelay, 1, cfg.baseDelay);

    GetWindowTextW(hMaxDelay_, buf, 32);
    cfg.maxDelay = buf[0] ? static_cast<int>(wcstol(buf, nullptr, 10)) : 80;
    cfg.maxDelay = (std::max)(cfg.maxDelay, cfg.minDelay);
    cfg.maxDelay = std::clamp(cfg.maxDelay, cfg.minDelay, 2000);

    // Push clamped values back to UI only if they are different to prevent cursor jump
    auto updateIfDiff = [](HWND hEdit, int val) {
        wchar_t txt[32] = {0};
        GetWindowTextW(hEdit, txt, 32);
        std::wstring valStr = std::to_wstring(val);
        if (valStr != txt) {
            DWORD start = 0, end = 0;
            SendMessageW(hEdit, EM_GETSEL, reinterpret_cast<WPARAM>(&start), reinterpret_cast<LPARAM>(&end));
            SetWindowTextW(hEdit, valStr.c_str());
            SendMessageW(hEdit, EM_SETSEL, start, end);
        }
    };
    updateIfDiff(hBaseDelay_, cfg.baseDelay);
    updateIfDiff(hMinDelay_,  cfg.minDelay);
    updateIfDiff(hMaxDelay_,  cfg.maxDelay);

    int sliderPos = static_cast<int>(SendMessageW(hSliderRandom_, TBM_GETPOS, 0, 0));
    cfg.randomization = sliderPos / 100.0f;

    // ── Trigger ──
    cfg.triggerMode = (SendMessageW(hRadioHold_, BM_GETCHECK, 0, 0) == BST_CHECKED)
                      ? TriggerMode::Hold : TriggerMode::Toggle;

    // ── Pattern ──
    int patIdx = static_cast<int>(SendMessageW(hComboPattern_, CB_GETCURSEL, 0, 0));
    if (patIdx >= 0 && patIdx < PATTERN_COUNT)
        cfg.pattern = static_cast<StrafePattern>(patIdx);

    cfg.fatigueEnabled = (SendMessageW(hCheckFatigue_, BM_GETCHECK, 0, 0) == BST_CHECKED);

    // ── Advanced ──
    GetWindowTextW(hEditTitle_, buf, 256);
    cfg.windowTitle = buf;

    cfg.alwaysOnTop    = (SendMessageW(hCheckOnTop_, BM_GETCHECK, 0, 0) == BST_CHECKED);
    cfg.minimizeToTray = (SendMessageW(hCheckTray_,  BM_GETCHECK, 0, 0) == BST_CHECKED);
    cfg.autoSprint     = (SendMessageW(hCheckAutoSprint_, BM_GETCHECK, 0, 0) == BST_CHECKED);

    // ── Apply ──
    config_.setConfig(cfg);

    // Reflect title and topmost immediately
    SetWindowTextW(hwnd_, cfg.windowTitle.c_str());
    SetWindowPos(hwnd_,
                 cfg.alwaysOnTop ? HWND_TOPMOST : HWND_NOTOPMOST,
                 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

    ignoreChanges_ = false;
}

void GUI::loadConfigToUI()
{
    ignoreChanges_ = true;  // suppress feedback loop

    auto cfg = config_.getConfig();

    SetWindowTextW(hBaseDelay_, std::to_wstring(cfg.baseDelay).c_str());
    SetWindowTextW(hMinDelay_,  std::to_wstring(cfg.minDelay).c_str());
    SetWindowTextW(hMaxDelay_,  std::to_wstring(cfg.maxDelay).c_str());

    SendMessageW(hSliderRandom_, TBM_SETPOS, TRUE,
                 static_cast<LPARAM>(cfg.randomization * 100.0f));
    updateRandomLabel();

    // Update trigger key button text
    SetWindowTextW(hBtnTriggerBind_, getKeyName(cfg.triggerKey).c_str());

    SendMessageW(hRadioHold_,   BM_SETCHECK,
                 cfg.triggerMode == TriggerMode::Hold ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessageW(hRadioToggle_, BM_SETCHECK,
                 cfg.triggerMode == TriggerMode::Toggle ? BST_CHECKED : BST_UNCHECKED, 0);

    SendMessageW(hComboPattern_, CB_SETCURSEL, static_cast<int>(cfg.pattern), 0);
    SendMessageW(hCheckFatigue_, BM_SETCHECK, cfg.fatigueEnabled ? BST_CHECKED : BST_UNCHECKED, 0);

    SetWindowTextW(hEditTitle_, cfg.windowTitle.c_str());
    SendMessageW(hCheckOnTop_, BM_SETCHECK, cfg.alwaysOnTop    ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessageW(hCheckTray_,  BM_SETCHECK, cfg.minimizeToTray ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessageW(hCheckAutoSprint_, BM_SETCHECK, cfg.autoSprint ? BST_CHECKED : BST_UNCHECKED, 0);

    ignoreChanges_ = false;
}

void GUI::updateRandomLabel()
{
    int pos = static_cast<int>(SendMessageW(hSliderRandom_, TBM_GETPOS, 0, 0));
    wchar_t buf[16];
    wsprintfW(buf, L"%d%%", pos);
    SetWindowTextW(hLabelRandom_, buf);
}

// ════════════════════════════════════════════════════════════════════
//  STATUS UPDATE
// ════════════════════════════════════════════════════════════════════

void GUI::updateStatus(bool active)
{
    macroActive_ = active;
    SetWindowTextW(hLabelStatus_, active ? L"\x25CF Active" : L"\x25CF Idle");
    InvalidateRect(hLabelStatus_, nullptr, TRUE);   // force repaint for colour change
}

// ════════════════════════════════════════════════════════════════════
//  TRAY ICON
// ════════════════════════════════════════════════════════════════════

void GUI::setupTrayIcon(HWND hwnd)
{
    if (trayAdded_) return;

    nid_ = {};
    nid_.cbSize           = sizeof(nid_);
    nid_.hWnd             = hwnd;
    nid_.uID              = 1;
    nid_.uFlags           = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid_.uCallbackMessage = WM_TRAYICON;
    nid_.hIcon            = LoadIconW(hInstance_, MAKEINTRESOURCEW(1));
    wcscpy_s(nid_.szTip, L"Strafe Macro");

    Shell_NotifyIconW(NIM_ADD, &nid_);
    trayAdded_ = true;
}

void GUI::removeTrayIcon()
{
    if (!trayAdded_) return;
    Shell_NotifyIconW(NIM_DELETE, &nid_);
    trayAdded_ = false;
}
