#pragma once
#ifndef HOTKEY_MANAGER_H
#define HOTKEY_MANAGER_H

#include "config.h"
#include <atomic>
#include <thread>
#include <functional>

// ─── Global hotkey detection via GetAsyncKeyState polling ───────────
//
//  Why polling instead of RegisterHotKey?
//  - RegisterHotKey creates a system-wide registration (leaves a trace).
//  - GetAsyncKeyState is a simple, stateless query with no side effects.
// ────────────────────────────────────────────────────────────────────

class HotkeyManager {
public:
    /// Callback signature: bool = true when macro should activate, false to deactivate.
    using Callback = std::function<void(bool)>;

    explicit HotkeyManager(ConfigManager& config);
    ~HotkeyManager();

    /// Begin polling in a background thread.
    void start(Callback callback);

    /// Stop the polling thread (blocks until joined).
    void stop();

    bool isRunning() const { return running_; }

private:
    void pollLoop();

    ConfigManager& config_;
    Callback       callback_;

    std::atomic<bool> running_{false};
    std::thread       pollThread_;

    // State for edge detection
    bool wasPressed_  = false;
    bool toggleState_ = false;
};

#endif // HOTKEY_MANAGER_H
