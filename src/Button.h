#pragma once

#include <Arduino.h>

namespace mrm {

// Single button reduced to one primitive: how many clicks happened in a burst.
// Callers map counts to actions (1 = next, 2 = menu, N = shortcut), so there is
// one debounce/counting path instead of separate single/double/long handling.
class Button {
public:
    struct Config {
        uint16_t debounceMs = 30;
        uint16_t gapMs = 360; // quiet time that closes a click burst
        bool activeLow = true;
    };

    explicit Button(uint8_t pin)
        : pin_(pin) {}
    Button(uint8_t pin, const Config& config)
        : pin_(pin)
        , config_(config) {}

    void begin() {
        pinMode(pin_, config_.activeLow ? INPUT_PULLUP : INPUT);
    }

    // Drop a burst in progress so clicks from one context do not leak into the next.
    void reset() {
        raw_ = false;
        stable_ = false;
        count_ = 0;
        edgeAt_ = lastChange_ = 0;
    }

    // Number of clicks in the burst that just ended, reported once the button has
    // been released for gapMs. Returns 0 the rest of the time.
    uint8_t clicks() {
        const uint32_t now = millis();
        const bool down = held();

        if (down != raw_) {
            raw_ = down;
            edgeAt_ = now;
        }
        if (now - edgeAt_ >= config_.debounceMs && down != stable_) {
            stable_ = down;
            lastChange_ = now;
            if (stable_ && count_ < 250)
                ++count_;
        }
        if (count_ > 0 && !stable_ && now - lastChange_ >= config_.gapMs) {
            const uint8_t n = count_;
            count_ = 0;
            return n;
        }
        return 0;
    }

private:
    bool held() const {
        const int level = digitalRead(pin_);
        return config_.activeLow ? level == LOW : level == HIGH;
    }

    uint8_t pin_;
    Config config_{};
    bool raw_ = false;
    bool stable_ = false;
    uint8_t count_ = 0;
    uint32_t edgeAt_ = 0;
    uint32_t lastChange_ = 0;
};

} // namespace mrm
