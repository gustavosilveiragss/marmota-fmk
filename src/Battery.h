#pragma once

#include <Arduino.h>

namespace mrm {

// Cell gauge behind a resistor divider. Each read is a trimmed mean (noisy ADC, high-impedance
// divider, radio bursts on the rail), smoothed across updates, and the percent only climbs when
// the cell really climbs, so the badge does not bounce.
class Battery {
public:
    struct Point {
        float volts;
        uint8_t percent;
    };

    struct Config {
        uint8_t adcPin = 3;   // ADC1 channel, safe alongside WiFi
        float divider = 2.0f; // 100k/100k halves the cell voltage
        uint8_t samples = 24; // per read; only the middle half is averaged (max 32)
        uint32_t intervalMs = 5000;
        uint8_t lowPct = 15;
        float calibration = 1.0f;
        float smoothing = 0.25f;       // weight of a fresh read in the running voltage
        float riseHysteresisV = 0.04f; // climb needed before the percent is allowed back up
        float usbThresholdV = 4.35f;   // above any real cell: the node is on the usb rail
        const Point* curve = nullptr;  // optional, highest voltage first, null uses the built-in LiPo curve
        uint8_t curveLen = 0;
    };

    Battery() = default;
    explicit Battery(const Config& config)
        : config_(config) {}

    void begin();
    bool update();

    float voltage() const { return voltage_; }
    uint8_t percent() const { return percent_; }
    bool low() const { return !usb_ && percent_ < config_.lowPct; }

    bool usb() const { return usb_; } // reading is the usb rail, not a cell

    uint16_t nodeMillivolts() const { return nodeMv_; }
    uint16_t readNodeMillivolts() const;

private:
    float readVoltage();
    uint8_t percentFromVoltage(float v) const;

    Config config_{};
    float voltage_ = 0.0f;
    float anchor_ = 0.0f; // voltage that set the current percent, the rise hysteresis reference
    uint16_t nodeMv_ = 0;
    uint8_t percent_ = 0;
    uint32_t lastRead_ = 0;
    bool primed_ = false;
    bool usb_ = false;
};

} // namespace mrm
