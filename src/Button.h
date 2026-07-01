#pragma once

#include <Arduino.h>

namespace mrm {

enum class Gesture : uint8_t { None,
                               Single,
                               Double,
                               Long };

class Button {
public:
    struct Config {
        uint16_t debounceMs = 30;
        uint16_t doubleGapMs = 360;
        uint16_t longPressMs = 700; // 0 disables long press
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

    Gesture poll() {
        const uint32_t now = millis();
        const bool down = held();

        if (down != raw_) {
            raw_ = down;
            edgeAt_ = now;
        }

        if (now - edgeAt_ >= config_.debounceMs && down != stable_) {
            stable_ = down;
            if (stable_)
                onPress(now);
            else if (Gesture g = onRelease(now); g != Gesture::None)
                return g;
        }

        if (config_.longPressMs && stable_ && !longFired_ && now - pressAt_ >= config_.longPressMs) {
            longFired_ = true;
            pendingSingle_ = false;
            return Gesture::Long;
        }

        if (pendingSingle_ && now - releaseAt_ >= config_.doubleGapMs) {
            pendingSingle_ = false;
            return Gesture::Single;
        }

        return Gesture::None;
    }

private:
    bool held() const {
        const int level = digitalRead(pin_);
        return config_.activeLow ? level == LOW : level == HIGH;
    }

    void onPress(uint32_t now) {
        pressAt_ = now;
        longFired_ = false;
    }

    Gesture onRelease(uint32_t now) {
        releaseAt_ = now;
        if (longFired_)
            return Gesture::None;
        if (pendingSingle_) {
            pendingSingle_ = false;
            return Gesture::Double;
        }
        pendingSingle_ = true;
        return Gesture::None;
    }

    uint8_t pin_;
    Config config_{};
    bool raw_ = false;
    bool stable_ = false;
    bool pendingSingle_ = false;
    bool longFired_ = false;
    uint32_t edgeAt_ = 0;
    uint32_t pressAt_ = 0;
    uint32_t releaseAt_ = 0;
};

} // namespace mrm
