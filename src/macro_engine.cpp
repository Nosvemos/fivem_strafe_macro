#include "macro_engine.h"
#include "process_monitor.h"
#include <random>

static std::mt19937& rng()
{
    static std::mt19937 gen(std::random_device{}());
    return gen;
}

MacroEngine::MacroEngine(ConfigManager& config)
    : config_(config)
    , humanizer_(config)
{}

MacroEngine::~MacroEngine()
{
    stop();
}

// ─── Start / Stop (serialised by controlMutex_) ────────────────────

void MacroEngine::start()
{
    std::lock_guard lk(controlMutex_);
    if (active_) return;

    active_ = true;
    humanizer_.reset();
    macroThread_ = std::thread(&MacroEngine::strafeLoop, this);
}

void MacroEngine::stop()
{
    std::lock_guard lk(controlMutex_);
    if (!active_) return;

    active_ = false;
    if (macroThread_.joinable()) {
        macroThread_.join();
    }
}

// ─── Core strafe loop ──────────────────────────────────────────────

void MacroEngine::strafeLoop()
{
    std::uniform_int_distribution<int> coinFlip(0, 1);
    std::uniform_int_distribution<int> startDelayDist(0, 400);
    std::uniform_int_distribution<int> wrongKeyDist(1, 100);
    std::uniform_int_distribution<int> wrongKeyChoose(0, 3);
    WORD wrongKeys[4] = {
        InputSender::SCAN_W,
        InputSender::SCAN_S,
        InputSender::SCAN_A,
        InputSender::SCAN_D,
    };

    // ── Random start delay (0-400ms, anti-bot) ──
    Sleep(startDelayDist(rng()));

    auto initCfg = config_.getConfig();
    bool didSprint = initCfg.autoSprint;

    // Press Shift to sprint
    if (didSprint) {
        InputSender::pressKey(InputSender::SCAN_SHIFT);
    }

    while (active_) {
        auto cfg = config_.getConfig();

        WORD keys[4];
        int  keyCount = 0;

        switch (cfg.pattern) {
            case StrafePattern::AD:
                keys[0] = InputSender::SCAN_A;
                keys[1] = InputSender::SCAN_D;
                keyCount = 2;
                break;
            case StrafePattern::ADA:
                keys[0] = InputSender::SCAN_A;
                keys[1] = InputSender::SCAN_D;
                keys[2] = InputSender::SCAN_A;
                keyCount = 3;
                break;
            case StrafePattern::OnlyA:
                keys[0] = InputSender::SCAN_A;
                keyCount = 1;
                break;
            case StrafePattern::OnlyD:
                keys[0] = InputSender::SCAN_D;
                keyCount = 1;
                break;
            case StrafePattern::WASD:
                keys[0] = InputSender::SCAN_W;
                keys[1] = InputSender::SCAN_A;
                keys[2] = InputSender::SCAN_S;
                keys[3] = InputSender::SCAN_D;
                keyCount = 4;
                break;
            case StrafePattern::WDSA:
                keys[0] = InputSender::SCAN_W;
                keys[1] = InputSender::SCAN_D;
                keys[2] = InputSender::SCAN_S;
                keys[3] = InputSender::SCAN_A;
                keyCount = 4;
                break;
            case StrafePattern::DSAW:
                keys[0] = InputSender::SCAN_D;
                keys[1] = InputSender::SCAN_S;
                keys[2] = InputSender::SCAN_A;
                keys[3] = InputSender::SCAN_W;
                keyCount = 4;
                break;
            case StrafePattern::WADS:
                keys[0] = InputSender::SCAN_W;
                keys[1] = InputSender::SCAN_A;
                keys[2] = InputSender::SCAN_D;
                keys[3] = InputSender::SCAN_S;
                keyCount = 4;
                break;
        }

        for (int i = 0; i < keyCount; ++i) {
            if (!active_) break;

            // ── Wrong key injection: ~4% chance per cycle (suppressed if randomization is 0) ──
            if (cfg.randomization > 0.0f && wrongKeyDist(rng()) <= 4 && keyCount > 0) {
                WORD badKey = wrongKeys[wrongKeyChoose(rng())];
                InputSender::pressKey(badKey);
                Sleep(static_cast<DWORD>(humanizer_.getPressDuration() * cfg.randomization));
                InputSender::releaseKey(badKey);
                Sleep(static_cast<DWORD>(50 * cfg.randomization));
            }

            int holdDur         = humanizer_.getNextDelay();
            int transitionDelay = 0;
            if (cfg.randomization > 0.0f) {
                transitionDelay = static_cast<int>(std::round(humanizer_.getPressDuration() * cfg.randomization));
            }

            InputSender::pressKey(keys[i]);
            Sleep(static_cast<DWORD>(holdDur));

            if (!active_) {
                InputSender::releaseKey(keys[i]);
                break;
            }

            InputSender::releaseKey(keys[i]);

            if (transitionDelay > 0) {
                Sleep(static_cast<DWORD>(transitionDelay));
            }

            if (cfg.randomization > 0.0f && humanizer_.shouldStutter()) {
                Sleep(static_cast<DWORD>(humanizer_.getStutterDuration()));
            }
        }
    }

    InputSender::releaseKey(InputSender::SCAN_W);
    InputSender::releaseKey(InputSender::SCAN_S);
    if (coinFlip(rng())) {
        InputSender::releaseKey(InputSender::SCAN_D);
        InputSender::releaseKey(InputSender::SCAN_A);
    } else {
        InputSender::releaseKey(InputSender::SCAN_A);
        InputSender::releaseKey(InputSender::SCAN_D);
    }
    if (didSprint) {
        InputSender::releaseKey(InputSender::SCAN_SHIFT);
    }
}
