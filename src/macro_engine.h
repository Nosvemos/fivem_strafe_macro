#pragma once
#ifndef MACRO_ENGINE_H
#define MACRO_ENGINE_H

#include "config.h"
#include "humanizer.h"
#include "input_sender.h"
#include <atomic>
#include <mutex>
#include <thread>

// ─── Strafe macro engine ────────────────────────────────────────────
//
//  Runs the A/D key-alternation loop on a dedicated thread.
//  All timing is governed by the Humanizer.
// ────────────────────────────────────────────────────────────────────

class MacroEngine {
public:
    explicit MacroEngine(ConfigManager& config);
    ~MacroEngine();

    /// Start the strafe loop (no-op if already running).
    void start();

    /// Stop the strafe loop and join the thread.
    void stop();

    bool isActive() const { return active_; }

private:
    void strafeLoop();

    ConfigManager& config_;
    Humanizer      humanizer_;

    std::atomic<bool> active_{false};
    std::mutex        controlMutex_;   // serialise start/stop calls
    std::thread       macroThread_;
};

#endif // MACRO_ENGINE_H
