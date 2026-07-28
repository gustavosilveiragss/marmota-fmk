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
constexpr uint8_t kMaxSamples = 32;
constexpr uint8_t kMinSamples = 4;

} // namespace

void Battery::begin() {
    analogReadResolution(kAdcBits);
    pinMode(config_.adcPin, INPUT);
    analogSetPinAttenuation(config_.adcPin, ADC_11db); // node sits at ~2.1V full, ~2.5V on usb
    voltage_ = readVoltage();
    anchor_ = voltage_;
    usb_ = voltage_ > config_.usbThresholdV;
    percent_ = percentFromVoltage(voltage_);
    lastRead_ = millis();
    primed_ = true;
}

bool Battery::update() {
    if (primed_ && millis() - lastRead_ < config_.intervalMs)
        return false;
    lastRead_ = millis();

    const float fresh = readVoltage();
    const float alpha = constrain(config_.smoothing, 0.0f, 1.0f);
    voltage_ = primed_ ? voltage_ + alpha * (fresh - voltage_) : fresh;
    usb_ = voltage_ > config_.usbThresholdV;

    // Falling always wins; rising needs real gain (charger, rail recovering after playback),
    // otherwise noise ratchets the badge between two curve points.
    const uint8_t reading = percentFromVoltage(voltage_);
    if (!primed_ || reading < percent_ || usb_ || voltage_ > anchor_ + config_.riseHysteresisV) {
        percent_ = reading;
        anchor_ = voltage_;
    }
    primed_ = true;
    return true;
}

// Trimmed mean of the middle half: TX bursts and the high divider impedance show up as one-sided
// spikes, which a plain mean folds in and a median answers with one sample's quantization.
uint16_t Battery::readNodeMillivolts() const {
    const uint8_t n = constrain(config_.samples, kMinSamples, kMaxSamples);
    uint16_t s[kMaxSamples];
    analogReadMilliVolts(config_.adcPin); // discard the settling conversion
    for (uint8_t i = 0; i < n; ++i)
        s[i] = uint16_t(analogReadMilliVolts(config_.adcPin));

    for (uint8_t i = 1; i < n; ++i) { // insertion sort, n <= 32
        const uint16_t v = s[i];
        int8_t j = int8_t(i) - 1;
        for (; j >= 0 && s[j] > v; --j)
            s[j + 1] = s[j];
        s[j + 1] = v;
    }

    const uint8_t drop = n / 4;
    const uint8_t kept = n - 2 * drop;
    uint32_t sum = 0;
    for (uint8_t i = drop; i < drop + kept; ++i)
        sum += s[i];
    return uint16_t((sum + kept / 2) / kept);
}

float Battery::calibrateTo(float cellVolts) {
    const float raw = (float(readNodeMillivolts()) / kMvPerV) * config_.divider;
    if (raw > 0.1f)
        config_.calibration = cellVolts / raw;
    voltage_ = cellVolts;
    anchor_ = cellVolts;
    percent_ = percentFromVoltage(cellVolts);
    return config_.calibration;
}

float Battery::readVoltage() {
    nodeMv_ = readNodeMillivolts();
    return (float(nodeMv_) / kMvPerV) * config_.divider * config_.calibration;
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
