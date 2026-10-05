#pragma once

#include <Arduino.h>

namespace mrm {

class Led {
public:
    struct Config {
        uint8_t pin;
        bool activeLow = true;
        uint32_t periodMs = 3000;
        uint16_t onMs = 30;
    };

    explicit Led(const Config& config)
        : config_(config) {}

    void begin();
    void on();
    void off();
    void heartbeat();

private:
    void write(bool lit);

    Config config_;
    uint32_t last_ = 0;
    bool lit_ = false;
};

namespace power {

void radioOff();
void cpuClock(uint32_t mhz);

// Light sleep por no máximo maxMs, acordando na próxima borda do wakePin (aperto e soltura)
// quando ele está setado. maxMs = 0 tira o timer: dorme até a borda (~130uA), sem reboot. Mantém
// a RAM e o conteúdo do display, então e o economizador de idle preferido.
void lightSleep(uint32_t maxMs, int wakePin = -1, bool activeLow = true);

void deepSleepOnButton(uint8_t wakePin, bool activeLow = true);

} // namespace power

} // namespace mrm
