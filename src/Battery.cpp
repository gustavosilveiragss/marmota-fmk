#include "Battery.h"

namespace mrm {

namespace {

struct Point {
    float volts;
    uint8_t percent;
};

// LiPo discharge curve, resting voltage to charge left.
constexpr Point kCurve[] = {
    {4.20f, 100},
    {4.10f, 90},
    {4.00f, 80},
    {3.90f, 70},
    {3.80f, 60},
    {3.70f, 50},
    {3.60f, 40},
    {3.50f, 30},
    {3.40f, 20},
    {3.30f, 10},
    {3.00f, 0},
};

} // namespace

void Battery::begin() {
    analogReadResolution(12);
    pinMode(config_.adcPin, INPUT);
    voltage_ = readVoltage();
    percent_ = percentFromVoltage(voltage_);
    lastRead_ = millis();
    primed_ = true;
}

bool Battery::update() {
    if (primed_ && millis() - lastRead_ < config_.intervalMs)
        return false;
    voltage_ = readVoltage();
    percent_ = percentFromVoltage(voltage_);
    lastRead_ = millis();
    primed_ = true;
    return true;
}

float Battery::readVoltage() const {
    uint32_t sum = 0;
    for (uint8_t i = 0; i < config_.samples; ++i)
        sum += analogReadMilliVolts(config_.adcPin);
    float millivolts = float(sum) / config_.samples;
    return (millivolts / 1000.0f) * config_.divider * config_.calibration;
}

uint8_t Battery::percentFromVoltage(float v) {
    if (v >= kCurve[0].volts)
        return 100;
    constexpr size_t n = sizeof(kCurve) / sizeof(kCurve[0]);
    if (v <= kCurve[n - 1].volts)
        return 0;
    for (size_t i = 1; i < n; ++i) {
        if (v >= kCurve[i].volts) {
            const Point& hi = kCurve[i - 1];
            const Point& lo = kCurve[i];
            float span = hi.volts - lo.volts;
            float frac = (v - lo.volts) / span;
            return uint8_t(lo.percent + frac * (hi.percent - lo.percent) + 0.5f);
        }
    }
    return 0;
}

} // namespace mrm
