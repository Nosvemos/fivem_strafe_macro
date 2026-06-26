#include "hotkey_manager.h"
#include <random>

static std::mt19937& rng()
{
    static std::mt19937 gen(std::random_device{}());
    return gen;
}

HotkeyManager::HotkeyManager(ConfigManager& config)
    : config_(config)
{}

HotkeyManager::~HotkeyManager()
{
    stop();
}

void HotkeyManager::start(Callback callback)
{
    if (running_) return;

    callback_    = std::move(callback);
    running_     = true;
    wasPressed_  = false;
    toggleState_ = false;
    pollThread_  = std::thread(&HotkeyManager::pollLoop, this);
}

void HotkeyManager::stop()
{
    running_ = false;
    if (pollThread_.joinable()) {
        pollThread_.join();
    }
}

// ─── Polling loop (~10 ms cadence) ─────────────────────────────────

void HotkeyManager::pollLoop()
{
    int unpressedTicks = 0;
    while (running_) {
        auto cfg = config_.getConfig();
        int  key = cfg.triggerKey;

        // Check physical key state
        bool isPressed = (GetAsyncKeyState(key) & 0x8000) != 0;

        if (cfg.triggerMode == TriggerMode::Hold) {
            // ── Hold mode: active while key is down ──
            if (isPressed) {
                unpressedTicks = 0;
                if (!wasPressed_) {
                    wasPressed_ = true;
                    if (callback_) callback_(true);
                }
            } else {
                if (wasPressed_) {
                    unpressedTicks++;
                    if (unpressedTicks >= 5) { // Requires ~50ms of consecutive released states
                        wasPressed_ = false;
                        unpressedTicks = 0;
                        if (callback_) callback_(false);
                    }
                }
            }
        } else {
            // ── Toggle mode: flip state on each press ──
            if (isPressed && !wasPressed_) {
                wasPressed_  = true;
                toggleState_ = !toggleState_;
                if (callback_) callback_(toggleState_);
            } else if (!isPressed) {
                wasPressed_ = false;
            }
        }

        std::uniform_int_distribution<int> sleepJitter(7, 15);
        Sleep(sleepJitter(rng()));
    }
}
