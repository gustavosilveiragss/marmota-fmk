#pragma once

#include <Arduino.h>

namespace mrm {

// Medidor de celula atras de um divisor resistivo. Cada leitura e uma media aparada (ADC ruidoso,
// divisor de alta impedancia, rajadas do radio no rail), suavizada entre updates, e a porcentagem
// so mexe quando a celula sai de uma banda morta, pra o badge nao balancar com o ruido.
class Battery {
public:
    struct Point {
        float volts;
        uint8_t percent;
    };

    struct Config {
        uint8_t adcPin = 3;   // canal do ADC1, seguro com o WiFi ligado
        float divider = 2.0f; // 100k/100k divide a tensao da celula por 2
        uint8_t samples = 24; // por leitura, so a metade do meio e mediada (max 32)
        uint32_t intervalMs = 5000;
        uint8_t lowPct = 15;
        float calibration = 1.0f;
        float smoothing = 0.25f;       // peso de uma leitura nova na tensao corrente
        float riseHysteresisV = 0.04f; // banda morta: a tensao move isso da ancora pra o badge mexer
        float usbThresholdV = 4.35f;   // acima de qualquer celula real, o no esta no rail do usb
        const Point* curve = nullptr;  // opcional, maior tensao primeiro, null usa a curva LiPo interna
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

    bool usb() const { return usb_; } // a leitura e o rail do usb, nao uma celula

    uint16_t nodeMillivolts() const { return nodeMv_; }
    uint16_t readNodeMillivolts() const;

    // Trim por unidade: divisor e ganho de ADC variam por placa. calibrateTo() com a celula em
    // repouso mede o valor, quem chama persiste o retorno.
    float calibration() const { return config_.calibration; }
    void setCalibration(float cal) { config_.calibration = cal; }
    float calibrateTo(float cellVolts);

private:
    float readVoltage();
    uint8_t percentFromVoltage(float v) const;

    Config config_{};
    float voltage_ = 0.0f;
    float anchor_ = 0.0f; // tensao que fixou a porcentagem vigente, centro da banda morta
    uint16_t nodeMv_ = 0;
    uint8_t percent_ = 0;
    uint32_t lastRead_ = 0;
    bool primed_ = false;
    bool usb_ = false;
};

} // namespace mrm
