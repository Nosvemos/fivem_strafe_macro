#include "humanizer.h"
#include <algorithm>
#include <cmath>

Humanizer::Humanizer(ConfigManager& config)
    : config_(config)
    , gen_(std::random_device{}())          // seed from HW entropy
    , startTime_(std::chrono::steady_clock::now())
{}

// ─── Layer 1: Gaussian delay + Layer 3: drift + Layer 5: fatigue ───

int Humanizer::getNextDelay()
{
    auto cfg = config_.getConfig();

    if (cfg.randomization > 0.0f) {
        // Coin-flip micro-jitter: 50% chance of 1-5 ms extra pause before delay
        std::uniform_int_distribution<int> coin(0, 1);
        if (coin(gen_)) {
            std::uniform_int_distribution<int> extra(1, 5);
            Sleep(extra(gen_));
        }

        // Mean = base delay shifted by current drift
        double mean  = static_cast<double>(cfg.baseDelay) + driftOffset_;
        double sigma = mean * static_cast<double>(cfg.randomization);

        std::normal_distribution<double> dist(mean, sigma);
        double delay = dist(gen_);

        // ── Fatigue: add 0-15 ms penalty over time ──
        if (cfg.fatigueEnabled) {
            auto elapsed = std::chrono::steady_clock::now() - startTime_;
            auto minutes = std::chrono::duration_cast<std::chrono::minutes>(elapsed).count();
            double penalty = (std::min)(static_cast<double>(minutes), 15.0);
            delay += penalty;
        }

        // Clamp to user-defined bounds
        delay = std::clamp(delay,
                           static_cast<double>(cfg.minDelay),
                           static_cast<double>(cfg.maxDelay));

        // ── Layer 3: periodic drift (every 15-30 cycles) ──
        ++cycleCount_;
        if (cycleCount_ >= nextDriftCycle_) {
            double range = cfg.baseDelay * 0.10;  // ±10 % of base
            std::uniform_real_distribution<double> driftDist(-range, range);
            driftOffset_ = driftDist(gen_);

            std::uniform_int_distribution<int> cycleDist(15, 30);
            nextDriftCycle_ = cycleCount_ + cycleDist(gen_);
        }

        return static_cast<int>(std::round(delay));
    } else {
        return cfg.baseDelay;
    }
}

// ─── Layer 2: micro-jitter on key-press duration ───────────────────

int Humanizer::getPressDuration()
{
    // Human finger hold time varies ~8-25 ms
    std::uniform_int_distribution<int> dist(8, 25);
    return dist(gen_);
}

// ─── Layer 4: rare stutter ─────────────────────────────────────────

bool Humanizer::shouldStutter()
{
    // ~3 % probability per cycle
    std::uniform_int_distribution<int> dist(1, 100);
    return dist(gen_) <= 3;
}

int Humanizer::getStutterDuration()
{
    // Brief hesitation: 30-80 ms
    std::uniform_int_distribution<int> dist(30, 80);
    return dist(gen_);
}

// ─── Reset (called on macro start/stop) ────────────────────────────

void Humanizer::reset()
{
    cycleCount_     = 0;
    nextDriftCycle_ = 20;
    driftOffset_    = 0.0;
    startTime_      = std::chrono::steady_clock::now();
}
