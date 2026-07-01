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
void lightSleep(uint32_t ms);
void deepSleepOnButton(uint8_t wakePin, bool activeLow = true);

} // namespace power

} // namespace mrm
