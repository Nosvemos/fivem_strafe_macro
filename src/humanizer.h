#pragma once
#ifndef HUMANIZER_H
#define HUMANIZER_H

#include "config.h"
#include <random>
#include <chrono>

// ─── Multi-layer humanization engine ────────────────────────────────
//
//  Layer 1 – Gaussian delay:       N(μ = baseDelay + drift, σ)
//  Layer 2 – Micro-jitter:         Uniform key-press duration [8-25 ms]
//  Layer 3 – Periodic drift:       Slow base-delay shift every 15-30 cycles
//  Layer 4 – Rare stutter:         ~3 % chance of extra micro-pause
//  Layer 5 – Fatigue (optional):   Gradual slowdown over minutes
// ────────────────────────────────────────────────────────────────────

class Humanizer {
public:
    explicit Humanizer(ConfigManager& config);

    /// Next randomized delay between key switches (ms).
    int getNextDelay();

    /// Randomized key-press hold duration (ms).
    int getPressDuration();

    /// True ~3 % of the time → inject a micro-pause.
    bool shouldStutter();

    /// Duration of the micro-pause if stutter triggers (ms).
    int getStutterDuration();

    /// Reset internal counters (call when macro stops/starts).
    void reset();

private:
    ConfigManager& config_;

    // PRNG – seeded from hardware entropy
    std::mt19937 gen_;

    // Drift tracking
    int    cycleCount_     = 0;
    int    nextDriftCycle_ = 20;
    double driftOffset_    = 0.0;

    // Fatigue tracking
    std::chrono::steady_clock::time_point startTime_;
};

#endif // HUMANIZER_H
