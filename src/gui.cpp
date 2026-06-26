#include "gui.h"
#include "process_monitor.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <tchar.h>
#include <dwmapi.h>
#include <algorithm>
#include <random>

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// ─── UTF-8 / Wstring conversion utilities ────────────────────────────

static std::string to_utf8(const std::wstring& wstr) {
    if (wstr.empty()) return "";
    int size = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string str(size, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &str[0], size, NULL, NULL);
    return str;
}

static std::wstring to_wstring(const std::string& str) {
    if (str.empty()) return L"";
    int size = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstr(size, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstr[0], size);
    return wstr;
}

// ─── Random class name generator ────────────────────────────────────

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

// ─── Construction / Destruction ─────────────────────────────────────

GUI::GUI(ConfigManager& config, MacroEngine& engine, HotkeyManager& hotkeyMgr)
    : config_(config), engine_(engine), hotkeyMgr_(hotkeyMgr)
{
    className_ = generateClassName();
}

GUI::~GUI()
{
    shutdown();
}

// ─── Initialize Window & DirectX 11 ─────────────────────────────────

bool GUI::init(HINSTANCE hInstance)
{
    hInstance_ = hInstance;

    // ── Window class definition ──
    WNDCLASSEXW wc = { sizeof(wc) };
    wc.cbSize        = sizeof(WNDCLASSEXW);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInstance;
    wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr; // Let DirectX paint
    wc.lpszClassName = className_.c_str();
    wc.hIcon         = LoadIconW(hInstance, MAKEINTRESOURCEW(1)); // Loaded from resource.rc

    if (!RegisterClassExW(&wc)) {
        return false;
    }

    // ── Window styles: fixed size ──
    DWORD style   = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    DWORD exStyle = WS_EX_APPWINDOW;
    RECT  rc      = { 0, 0, 460, 520 }; // Clean size for controls
    AdjustWindowRectEx(&rc, style, FALSE, exStyle);

    auto cfg = config_.getConfig();
    macroActive_ = false;

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

    // ── Initialize DX11 ──
    if (!createDeviceD3D(hwnd_)) {
        cleanupDeviceD3D();
        UnregisterClassW(className_.c_str(), hInstance_);
        return false;
    }

    // ── Setup Dear ImGui context ──
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable keyboard navigation

    // ── Initialize ImGui Win32 + DX11 backends ──
    ImGui_ImplWin32_Init(hwnd_);
    ImGui_ImplDX11_Init(pd3dDevice_, pd3dDeviceContext_);

    // ── Apply gaming theme ──
    setupTheme();

    ShowWindow(hwnd_, SW_SHOW);
    UpdateWindow(hwnd_);

    return true;
}

// ─── Main Run Loop ──────────────────────────────────────────────────

void GUI::run()
{
    MSG msg;
    ZeroMemory(&msg, sizeof(msg));
    while (msg.message != WM_QUIT)
    {
        // Poll messages
        if (PeekMessageW(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
            continue;
        }

        // Render frame
        renderFrame();
    }
}

// ─── Shutdown & Cleanup ─────────────────────────────────────────────

void GUI::shutdown()
{
    removeTrayIcon();

    // Cleanup ImGui
    if (ImGui::GetCurrentContext()) {
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
    }

    // Cleanup DX11 Device
    cleanupDeviceD3D();

    if (hwnd_) {
        DestroyWindow(hwnd_);
        hwnd_ = nullptr;
    }

    if (hInstance_ && !className_.empty()) {
        UnregisterClassW(className_.c_str(), hInstance_);
        className_.clear();
    }
}

void GUI::updateStatus(bool active)
{
    macroActive_ = active;
}

// ─── Frame Rendering Loop ───────────────────────────────────────────

void GUI::renderFrame()
{
    // Start ImGui frame
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    // Set layout covering window client area
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
                             ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                             ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    ImGui::Begin("##Canvas", nullptr, flags);

    auto cfg = config_.getConfig();

    // ─── Header Section ───
    ImGui::TextColored(ImVec4(0.49f, 0.36f, 1.00f, 1.00f), "STRAFE MACRO");
    ImGui::SameLine();
    ImGui::TextDisabled("v1.0 · C++ · by Nosvemos");
    ImGui::SameLine(ImGui::GetIO().DisplaySize.x - 110);
    
    // Status Pill
    if (macroActive_) {
        ImGui::TextColored(ImVec4(0.13f, 0.77f, 0.37f, 1.00f), "[ ACTIVE ]");
    } else {
        ImGui::TextDisabled("[ IDLE ]");
    }

    ImGui::Separator();
    ImGui::Spacing();

    // ─── Tab Bar Layout ───
    if (ImGui::BeginTabBar("##SettingsTabs", ImGuiTabBarFlags_None))
    {
        // ── TAB 1: Movement & Timing ──
        if (ImGui::BeginTabItem("Movement"))
        {
            ImGui::Spacing();
            ImGui::TextDisabled("Configure delay parameters (ms):");

            // Slider: Base Delay
            int baseVal = cfg.baseDelay;
            if (ImGui::SliderInt("Base Delay", &baseVal, 1, 1000, "%d ms")) {
                cfg.baseDelay = baseVal;
                cfg.minDelay = (std::min)(cfg.minDelay, cfg.baseDelay);
                cfg.maxDelay = (std::max)(cfg.maxDelay, cfg.minDelay);
                config_.setConfig(cfg);
            }
            ImGui::SetItemTooltip("Target speed duration between keystrokes.");

            // Slider: Min Delay
            int minVal = cfg.minDelay;
            if (ImGui::SliderInt("Min Delay", &minVal, 1, cfg.baseDelay, "%d ms")) {
                cfg.minDelay = minVal;
                config_.setConfig(cfg);
            }
            ImGui::SetItemTooltip("Minimum boundary for randomized keys.");

            // Slider: Max Delay
            int maxVal = cfg.maxDelay;
            if (ImGui::SliderInt("Max Delay", &maxVal, cfg.minDelay, 2000, "%d ms")) {
                cfg.maxDelay = maxVal;
                config_.setConfig(cfg);
            }
            ImGui::SetItemTooltip("Maximum boundary for randomized keys.");

            // Slider: Randomization
            int randPos = static_cast<int>(cfg.randomization * 100.0f);
            if (ImGui::SliderInt("Jitter / Random", &randPos, 0, 100, "%d%%")) {
                cfg.randomization = randPos / 100.0f;
                config_.setConfig(cfg);
            }
            ImGui::SetItemTooltip("Gaussian randomness intensity. Set to 0% for pure static timings.");

            // Checkbox: Fatigue Simulation
            bool fatigue = cfg.fatigueEnabled;
            if (ImGui::Checkbox("Fatigue Simulation", &fatigue)) {
                cfg.fatigueEnabled = fatigue;
                config_.setConfig(cfg);
            }
            ImGui::SetItemTooltip("Gradually slows down movement speeds over time to mimic real finger fatigue.");

            ImGui::EndTabItem();
        }

        // ── TAB 2: Pattern & Trigger ──
        if (ImGui::BeginTabItem("Trigger"))
        {
            ImGui::Spacing();

            // Combo: Strafe Pattern Selection
            const char* patternNames[] = {
                "A <-> D (Classic)",
                "A -> D -> A (Triple)",
                "Strafe Left Only (A)",
                "Strafe Right Only (D)",
                "W -> A -> S -> D",
                "W -> D -> S -> A",
                "D -> S -> A -> W",
                "W -> A -> D -> S"
            };
            int curPattern = static_cast<int>(cfg.pattern);
            if (ImGui::Combo("Strafe Pattern", &curPattern, patternNames, IM_ARRAYSIZE(patternNames))) {
                cfg.pattern = static_cast<StrafePattern>(curPattern);
                config_.setConfig(cfg);
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // Trigger Mode Radio Buttons
            ImGui::Text("Trigger Mode:");
            int mode = static_cast<int>(cfg.triggerMode);
            if (ImGui::RadioButton("Hold", &mode, 0)) {
                cfg.triggerMode = TriggerMode::Hold;
                config_.setConfig(cfg);
            }
            ImGui::SameLine();
            if (ImGui::RadioButton("Toggle", &mode, 1)) {
                cfg.triggerMode = TriggerMode::Toggle;
                config_.setConfig(cfg);
            }

            ImGui::Spacing();

            // Key Binder Button
            ImGui::Text("Activation Key:");
            std::string bindText = isBinding_ ? "[Press any key...]" : to_utf8(getKeyName(cfg.triggerKey));
            
            if (ImGui::Button(bindText.c_str(), ImVec2(200, 30))) {
                isBinding_ = true;
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Click to assign a trigger key (supports Mouse 4/5, CapsLock, F-keys, etc.)");
            }

            // Key capture logic
            if (isBinding_) {
                for (int key = 1; key < 256; key++) {
                    if (key == VK_LBUTTON) continue; // Prevent binding to left-click selector
                    if (GetAsyncKeyState(key) & 0x8000) {
                        isBinding_ = false;
                        if (key != VK_ESCAPE) { // Escape cancels
                            config_.setTriggerKey(key);
                        }
                        break;
                    }
                }
            }

            ImGui::EndTabItem();
        }

        // ── TAB 3: Advanced Options ──
        if (ImGui::BeginTabItem("Advanced"))
        {
            ImGui::Spacing();

            // Title Editing
            char titleBuf[256] = {0};
            std::string utf8Title = to_utf8(cfg.windowTitle);
            strncpy_s(titleBuf, utf8Title.c_str(), sizeof(titleBuf) - 1);
            if (ImGui::InputText("Window Title", titleBuf, sizeof(titleBuf))) {
                cfg.windowTitle = to_wstring(titleBuf);
                config_.setConfig(cfg);
                SetWindowTextW(hwnd_, cfg.windowTitle.c_str());
            }
            ImGui::SetItemTooltip("Customize executable window header dynamically (evades window detection hooks).");

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // Checkbox: Always On Top
            bool onTop = cfg.alwaysOnTop;
            if (ImGui::Checkbox("Always on Top", &onTop)) {
                cfg.alwaysOnTop = onTop;
                config_.setConfig(cfg);
                SetWindowPos(hwnd_, onTop ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
            }

            // Checkbox: Minimize to Tray
            bool toTray = cfg.minimizeToTray;
            if (ImGui::Checkbox("Minimize to SysTray", &toTray)) {
                cfg.minimizeToTray = toTray;
                config_.setConfig(cfg);
            }
            ImGui::SetItemTooltip("Hides window into Taskbar Tray on click of minimize.");

            // Checkbox: Auto Sprint (Shift) - Optional
            bool sprint = cfg.autoSprint;
            if (ImGui::Checkbox("Auto Sprint (Left Shift)", &sprint)) {
                cfg.autoSprint = sprint;
                config_.setConfig(cfg);
            }
            ImGui::SetItemTooltip("Automatically presses and releases Left Shift to enforce running animation.");

            ImGui::EndTabItem();
        }

        // ── TAB 4: Anti-Cheat Status ──
        if (ImGui::BeginTabItem("Stealth"))
        {
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.49f, 0.36f, 1.00f, 1.00f), "Stealth Defenses Enabled:");

            ImGui::BulletText("Random Window Class Name: active");
            ImGui::BulletText("Low-Level Hardware Scan Codes: active");
            ImGui::BulletText("Gaussian Key Jitter: active");
            ImGui::BulletText("Dynamic Target Window Title: active");

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::TextWrapped("Pro Tip: Rename the compiled executable (e.g. 'lgHubHelper.exe') to bypass process name signature blacklists.");

            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // ─── Action Buttons ───
    ImVec2 btnSize(205, 45);

    // START (Green Theme override)
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.13f, 0.55f, 0.27f, 0.80f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.13f, 0.77f, 0.37f, 1.00f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.10f, 0.45f, 0.22f, 1.00f));
    if (ImGui::Button("START", btnSize)) {
        engine_.start();
        updateStatus(true);
    }
    ImGui::PopStyleColor(3);

    ImGui::SameLine();

    // STOP (Red Theme override)
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.70f, 0.20f, 0.20f, 0.80f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.90f, 0.27f, 0.27f, 1.00f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.55f, 0.15f, 0.15f, 1.00f));
    if (ImGui::Button("STOP", btnSize)) {
        engine_.stop();
        updateStatus(false);
    }
    ImGui::PopStyleColor(3);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // ─── Footer Process Status ───
    bool fivemUp = ProcessMonitor::isFiveMRunning();
    bool fg      = ProcessMonitor::isFiveMForeground();

    if (fivemUp) {
        if (fg) {
            ImGui::TextColored(ImVec4(0.13f, 0.77f, 0.37f, 1.00f), "FiveM Process: Active (Foreground)");
        } else {
            ImGui::TextColored(ImVec4(0.90f, 0.70f, 0.00f, 1.00f), "FiveM Process: Background (Minimized)");
        }
    } else {
        ImGui::TextColored(ImVec4(0.60f, 0.60f, 0.70f, 0.60f), "FiveM Process: Not Detected");
    }

    ImGui::End();

    // Render ImGui onto DirectX
    ImGui::Render();
    const float clear_color_with_alpha[4] = { 0.06f, 0.06f, 0.10f, 1.00f };
    pd3dDeviceContext_->OMSetRenderTargets(1, &mainRenderTargetView_, nullptr);
    pd3dDeviceContext_->ClearRenderTargetView(mainRenderTargetView_, clear_color_with_alpha);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    // Swap buffers with VSync enabled (Present parameter = 1) to prevent high CPU/GPU load
    pSwapChain_->Present(1, 0);
}

// ─── Key Name Translation Helper ────────────────────────────────────

std::wstring GUI::getKeyName(int vk)
{
    if (vk == VK_XBUTTON1) return L"Mouse 4";
    if (vk == VK_XBUTTON2) return L"Mouse 5";
    if (vk == VK_LBUTTON) return L"Left Click";
    if (vk == VK_RBUTTON) return L"Right Click";
    if (vk == VK_MBUTTON) return L"Middle Click";

    wchar_t name[64] = {};
    UINT scanCode = MapVirtualKeyW(vk, MAPVK_VK_TO_VSC);
    
    // Add extension bit if needed
    switch (vk) {
        case VK_LEFT: case VK_UP: case VK_RIGHT: case VK_DOWN:
        case VK_PRIOR: case VK_NEXT:
        case VK_END: case VK_HOME:
        case VK_INSERT: case VK_DELETE:
        case VK_DIVIDE: case VK_NUMLOCK:
            scanCode |= 0x100;
            break;
    }

    if (GetKeyNameTextW(scanCode << 16, name, 64) > 0) {
        return name;
    }

    // Fallback
    wchar_t buf[32];
    wsprintfW(buf, L"Key 0x%02X", vk);
    return buf;
}

// ─── System Tray Icon Setup ─────────────────────────────────────────

void GUI::setupTrayIcon()
{
    if (trayAdded_) return;

    nid_ = {};
    nid_.cbSize           = sizeof(nid_);
    nid_.hWnd             = hwnd_;
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

// ─── D3D11 Device / Context Initialization ──────────────────────────

bool GUI::createDeviceD3D(HWND hWnd)
{
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &pSwapChain_, &pd3dDevice_, &featureLevel, &pd3dDeviceContext_);
    if (res == DXGI_ERROR_UNSUPPORTED) // Fallback to WARP driver if hardware is unsupported
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &pSwapChain_, &pd3dDevice_, &featureLevel, &pd3dDeviceContext_);
    if (res != S_OK)
        return false;

    createRenderTarget();
    return true;
}

void GUI::cleanupDeviceD3D()
{
    cleanupRenderTarget();
    if (pSwapChain_) { pSwapChain_->Release(); pSwapChain_ = nullptr; }
    if (pd3dDeviceContext_) { pd3dDeviceContext_->Release(); pd3dDeviceContext_ = nullptr; }
    if (pd3dDevice_) { pd3dDevice_->Release(); pd3dDevice_ = nullptr; }
}

void GUI::createRenderTarget()
{
    ID3D11Texture2D* pBackBuffer = nullptr;
    pSwapChain_->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    if (pBackBuffer) {
        pd3dDevice_->CreateRenderTargetView(pBackBuffer, nullptr, &mainRenderTargetView_);
        pBackBuffer->Release();
    }
}

void GUI::cleanupRenderTarget()
{
    if (mainRenderTargetView_) { mainRenderTargetView_->Release(); mainRenderTargetView_ = nullptr; }
}

// ─── WndProc / handleMessage Window Hook ────────────────────────────

LRESULT CALLBACK GUI::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    GUI* self = nullptr;
    if (msg == WM_NCCREATE) {
        CREATESTRUCT* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
        self = reinterpret_cast<GUI*>(cs->lpCreateParams);
        SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        self->hwnd_ = hWnd;
    } else {
        self = reinterpret_cast<GUI*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));
    }

    if (self) {
        return self->handleMessage(hWnd, msg, wParam, lParam);
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

LRESULT GUI::handleMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_SIZE:
        if (pd3dDevice_ && wParam != SIZE_MINIMIZED)
        {
            cleanupRenderTarget();
            pSwapChain_->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            createRenderTarget();
        }
        if (wParam == SIZE_MINIMIZED && config_.getConfig().minimizeToTray)
        {
            ShowWindow(hWnd, SW_HIDE);
            setupTrayIcon();
        }
        return 0;

    case WM_TRAYICON:
        if (LOWORD(lParam) == WM_LBUTTONDBLCLK || LOWORD(lParam) == WM_RBUTTONUP)
        {
            ShowWindow(hWnd, SW_SHOW);
            ShowWindow(hWnd, SW_RESTORE);
            SetForegroundWindow(hWnd);
            removeTrayIcon();
        }
        return 0;

    case WM_MACRO_STATE:
        updateStatus(wParam == 1);
        return 0;

    case WM_TIMER:
        // Polling checks triggered by main thread timer (every 2s) to force UI status updates
        if (wParam == 1) {
            // Trigger refresh (handled naturally by ImGui frame render)
        }
        return 0;

    case WM_CLOSE:
        shutdown();
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

// ─── Theme Colors System Setup ──────────────────────────────────────

void GUI::setupTheme()
{
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    style.WindowRounding = 10.0f;
    style.FrameRounding = 5.0f;
    style.GrabRounding = 4.0f;
    style.PopupRounding = 5.0f;
    style.ScrollbarRounding = 4.0f;
    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 1.0f;
    style.WindowPadding = ImVec2(16.0f, 16.0f);
    style.ItemSpacing = ImVec2(12.0f, 10.0f);

    // Deep Dark Theme with Sleek Purple Accents
    colors[ImGuiCol_Text]                   = ImVec4(0.92f, 0.92f, 0.96f, 1.00f);
    colors[ImGuiCol_TextDisabled]           = ImVec4(0.45f, 0.45f, 0.55f, 1.00f);
    colors[ImGuiCol_WindowBg]               = ImVec4(0.06f, 0.06f, 0.08f, 1.00f); // #0A0A0D
    colors[ImGuiCol_ChildBg]                = ImVec4(0.09f, 0.09f, 0.12f, 1.00f); // #171A1F
    colors[ImGuiCol_PopupBg]                = ImVec4(0.08f, 0.08f, 0.10f, 1.00f);
    colors[ImGuiCol_Border]                 = ImVec4(0.18f, 0.18f, 0.22f, 1.00f); // #2A2F36
    colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg]                = ImVec4(0.11f, 0.11f, 0.16f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.15f, 0.15f, 0.22f, 1.00f);
    colors[ImGuiCol_FrameBgActive]          = ImVec4(0.18f, 0.18f, 0.28f, 1.00f);
    colors[ImGuiCol_TitleBg]                = ImVec4(0.06f, 0.06f, 0.08f, 1.00f);
    colors[ImGuiCol_TitleBgActive]          = ImVec4(0.06f, 0.06f, 0.08f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.06f, 0.06f, 0.08f, 1.00f);
    colors[ImGuiCol_MenuBarBg]              = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.02f, 0.02f, 0.04f, 0.50f);
    colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.25f, 0.25f, 0.30f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.35f, 0.35f, 0.40f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.45f, 0.45f, 0.50f, 1.00f);
    colors[ImGuiCol_CheckMark]              = ImVec4(0.49f, 0.36f, 1.00f, 1.00f); // #7C5CFF Purple
    colors[ImGuiCol_SliderGrab]             = ImVec4(0.49f, 0.36f, 1.00f, 1.00f);
    colors[ImGuiCol_SliderGrabActive]        = ImVec4(0.58f, 0.47f, 1.00f, 1.00f);
    colors[ImGuiCol_Button]                 = ImVec4(0.49f, 0.36f, 1.00f, 0.60f);
    colors[ImGuiCol_ButtonHovered]          = ImVec4(0.49f, 0.36f, 1.00f, 0.90f);
    colors[ImGuiCol_ButtonActive]           = ImVec4(0.38f, 0.24f, 0.90f, 1.00f);
    colors[ImGuiCol_Header]                 = ImVec4(0.49f, 0.36f, 1.00f, 0.30f);
    colors[ImGuiCol_HeaderHovered]          = ImVec4(0.49f, 0.36f, 1.00f, 0.60f);
    colors[ImGuiCol_HeaderActive]           = ImVec4(0.49f, 0.36f, 1.00f, 0.90f);
    colors[ImGuiCol_Separator]              = ImVec4(0.18f, 0.18f, 0.22f, 1.00f);
    colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.26f, 0.26f, 0.35f, 1.00f);
    colors[ImGuiCol_SeparatorActive]        = ImVec4(0.38f, 0.24f, 0.90f, 1.00f);
    colors[ImGuiCol_ResizeGrip]             = ImVec4(0.26f, 0.59f, 0.98f, 0.20f);
    colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
    colors[ImGuiCol_Tab]                    = ImVec4(0.11f, 0.11f, 0.16f, 1.00f);
    colors[ImGuiCol_TabHovered]             = ImVec4(0.49f, 0.36f, 1.00f, 0.80f);
    colors[ImGuiCol_TabActive]              = ImVec4(0.49f, 0.36f, 1.00f, 1.00f);
    colors[ImGuiCol_TabUnfocused]           = ImVec4(0.11f, 0.11f, 0.16f, 1.00f);
    colors[ImGuiCol_TabUnfocusedActive]     = ImVec4(0.15f, 0.15f, 0.22f, 1.00f);
    colors[ImGuiCol_PlotLines]              = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered]       = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotHistogram]          = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.49f, 0.36f, 1.00f, 0.35f);
    colors[ImGuiCol_DragDropTarget]         = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    colors[ImGuiCol_NavHighlight]           = ImVec4(0.49f, 0.36f, 1.00f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
}
