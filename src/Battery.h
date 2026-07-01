#pragma once

#include <Arduino.h>

namespace mrm {

class Battery {
public:
    struct Point {
        float volts;
        uint8_t percent;
    };

    struct Config {
        uint8_t adcPin = 3;   // ADC1 channel, safe alongside WiFi
        float divider = 2.0f; // 100k/100k halves the cell voltage
        uint8_t samples = 16;
        uint32_t intervalMs = 30000;
        uint8_t lowPct = 15;
        float calibration = 1.0f;
        const Point* curve = nullptr; // optional, highest voltage first; null uses the built-in LiPo curve
        uint8_t curveLen = 0;
    };

    Battery() = default;
    explicit Battery(const Config& config)
        : config_(config) {}

    void begin();
    bool update();

    float voltage() const { return voltage_; }
    uint8_t percent() const { return percent_; }
    bool low() const { return percent_ < config_.lowPct; }

private:
    float readVoltage() const;
    uint8_t percentFromVoltage(float v) const;

    Config config_{};
    float voltage_ = 0.0f;
    uint8_t percent_ = 0;
    uint32_t lastRead_ = 0;
    bool primed_ = false;
};

} // namespace mrm
