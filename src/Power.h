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

// Light sleep por no maximo maxMs, acordando cedo no wakePin (por nivel) quando ele esta
// setado. maxMs = 0 tira o timer: dorme ate o pino disparar (~130uA), sem reboot. Mantem a
// RAM e o conteudo do display, entao e o economizador de idle preferido.
void lightSleep(uint32_t maxMs, int wakePin = -1, bool activeLow = true);

void deepSleepOnButton(uint8_t wakePin, bool activeLow = true);

} // namespace power

} // namespace mrm
