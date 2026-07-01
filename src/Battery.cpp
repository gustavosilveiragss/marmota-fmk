#include "Battery.h"

namespace mrm {

namespace {

constexpr Battery::Point kCurve[] = {
    {4.20f, 100},
    {4.00f, 80},
    {3.85f, 60},
    {3.70f, 45},
    {3.55f, 25},
    {3.40f, 10},
    {3.30f, 5},
    {3.00f, 0},
};
constexpr size_t kCurveLen = sizeof(kCurve) / sizeof(kCurve[0]);
constexpr uint8_t kAdcBits = 12;
constexpr float kMvPerV = 1000.0f;

} // namespace

void Battery::begin() {
    analogReadResolution(kAdcBits);
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
    const float millivolts = float(sum) / config_.samples;
    return (millivolts / kMvPerV) * config_.divider * config_.calibration;
}

uint8_t Battery::percentFromVoltage(float v) const {
    const Point* curve = config_.curve ? config_.curve : kCurve;
    const size_t len = config_.curve ? config_.curveLen : kCurveLen;
    if (len == 0 || v >= curve[0].volts)
        return 100;
    if (v <= curve[len - 1].volts)
        return 0;
    for (size_t i = 1; i < len; ++i) {
        if (v >= curve[i].volts) {
            const Point& hi = curve[i - 1];
            const Point& lo = curve[i];
            const float frac = (v - lo.volts) / (hi.volts - lo.volts);
            return uint8_t(lroundf(lo.percent + frac * (hi.percent - lo.percent)));
        }
    }
    return 0;
}

} // namespace mrm
