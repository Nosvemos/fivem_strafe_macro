#pragma once
#ifndef CONFIG_H
#define CONFIG_H

#include <windows.h>
#include <mutex>
#include <shared_mutex>
#include <string>

// ─── Enums ──────────────────────────────────────────────────────────

enum class TriggerMode : int {
    Hold   = 0,   // Active while key is held
    Toggle = 1    // Press once to start, again to stop
};

enum class StrafePattern : int {
    AD    = 0,    // A ↔ D  (classic)
    ADA   = 1,    // A → D → A  (triple)
    OnlyA = 2,    // Strafe left only
    OnlyD = 3,    // Strafe right only
    WASD  = 4,    // W → A → S → D
    WDSA  = 5,    // W → D → S → A
    DSAW  = 6,    // D → S → A → W
    WADS  = 7,    // W → A → D → S
};

// ─── Configuration data (memory-only, NEVER written to disk) ────────

struct MacroConfig {
    int   baseDelay      = 120;             // Target delay between keys (ms)
    int   minDelay       = 80;              // Minimum allowed delay (ms)
    int   maxDelay       = 160;             // Maximum allowed delay (ms)
    float randomization  = 0.20f;           // Gaussian σ factor (0.0 – 1.0)

    int          triggerKey  = VK_XBUTTON1; // Mouse4
    TriggerMode  triggerMode = TriggerMode::Hold;
    StrafePattern pattern    = StrafePattern::AD;

    bool  fatigueEnabled  = false;          // Simulate human fatigue over time
    bool  alwaysOnTop     = true;
    bool  minimizeToTray  = true;
    bool  autoSprint      = true;
    std::wstring windowTitle = L"Volume Control";
};

// ─── Thread-safe configuration manager ──────────────────────────────

class ConfigManager {
public:
    ConfigManager() = default;

    /* ── Bulk access ─────────────────────────────────────────────── */
    MacroConfig getConfig() const {
        std::shared_lock lk(mutex_);
        return config_;
    }
    void setConfig(const MacroConfig& cfg) {
        std::unique_lock lk(mutex_);
        config_ = cfg;
    }

    /* ── Individual setters (called from GUI thread) ─────────────── */
    void setBaseDelay(int ms)               { std::unique_lock lk(mutex_); config_.baseDelay      = ms;  }
    void setMinDelay(int ms)                { std::unique_lock lk(mutex_); config_.minDelay       = ms;  }
    void setMaxDelay(int ms)                { std::unique_lock lk(mutex_); config_.maxDelay       = ms;  }
    void setRandomization(float pct)        { std::unique_lock lk(mutex_); config_.randomization  = pct; }
    void setTriggerKey(int vk)              { std::unique_lock lk(mutex_); config_.triggerKey     = vk;  }
    void setTriggerMode(TriggerMode m)      { std::unique_lock lk(mutex_); config_.triggerMode    = m;   }
    void setPattern(StrafePattern p)        { std::unique_lock lk(mutex_); config_.pattern        = p;   }
    void setFatigueEnabled(bool e)          { std::unique_lock lk(mutex_); config_.fatigueEnabled = e;   }
    void setAlwaysOnTop(bool e)             { std::unique_lock lk(mutex_); config_.alwaysOnTop    = e;   }
    void setMinimizeToTray(bool e)          { std::unique_lock lk(mutex_); config_.minimizeToTray = e;   }
    void setAutoSprint(bool e)              { std::unique_lock lk(mutex_); config_.autoSprint     = e;   }
    void setWindowTitle(const std::wstring& t) { std::unique_lock lk(mutex_); config_.windowTitle = t;   }

private:
    mutable std::shared_mutex mutex_;
    MacroConfig config_;
};

#endif // CONFIG_H
