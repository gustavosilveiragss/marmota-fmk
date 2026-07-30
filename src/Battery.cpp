#include "Battery.h"

namespace mrm {

namespace {

// Tensao de repouso (OCV) de LiPo LiCoO2 a 25C. Curvas de datasheet sao levantadas a 0.2C e
// ficam ~80mV abaixo da OCV. Usa-las num aparelho que puxa 0.005-0.02C superestima a carga em
// 15-25pp no meio da descarga. O 0% fica em 3.30V: abaixo disso o LDO da placa nao segura o
// 3V3 e sobra ~1% de carga real.
constexpr Battery::Point kCurve[] = {
    {4.20f, 100}, {4.15f, 95}, {4.11f, 90}, {4.08f, 85}, {4.02f, 80}, {3.98f, 75}, {3.95f, 70},
    {3.91f, 65},  {3.87f, 60}, {3.85f, 55}, {3.84f, 50}, {3.82f, 45}, {3.80f, 40}, {3.79f, 35},
    {3.77f, 30},  {3.75f, 25}, {3.73f, 20}, {3.71f, 15}, {3.69f, 10}, {3.61f, 5},  {3.30f, 0},
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
    analogSetPinAttenuation(config_.adcPin, ADC_11db); // no fica em ~2.1V cheio, ~2.5V no usb
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

    // A porcentagem so mexe quando a tensao sai da banda morta em volta da ancora, pros dois lados.
    // Ruido menor que a histerese nao balanca o badge entre pontos da curva, e uma queda de ruido
    // nao afunda a ancora, senao a porcentagem viraria o minimo corrente e nunca voltava a subir.
    const uint8_t reading = percentFromVoltage(voltage_);
    const bool moved = voltage_ > anchor_ + config_.riseHysteresisV ||
                       voltage_ < anchor_ - config_.riseHysteresisV;
    if (!primed_ || usb_ || moved) {
        percent_ = reading;
        anchor_ = voltage_;
    }
    primed_ = true;
    return true;
}

// Media aparada da metade do meio: rajadas de TX e a alta impedancia do divisor viram picos de um
// lado so, que a media simples engole e a mediana responde com a quantizacao de uma amostra.
uint16_t Battery::readNodeMillivolts() const {
    const uint8_t n = constrain(config_.samples, kMinSamples, kMaxSamples);
    uint16_t s[kMaxSamples];
    analogReadMilliVolts(config_.adcPin); // descarta a conversao de assentamento
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
