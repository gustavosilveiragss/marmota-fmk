#include "Power.h"

#include <WiFi.h>
#include <esp_sleep.h>
#include <driver/gpio.h>

namespace mrm {

void Led::begin() {
    pinMode(config_.pin, OUTPUT);
    off();
}

void Led::write(bool lit) {
    lit_ = lit;
    const bool level = config_.activeLow ? !lit : lit;
    digitalWrite(config_.pin, level ? HIGH : LOW);
}

void Led::on() {
    write(true);
}

void Led::off() {
    write(false);
}

void Led::heartbeat() {
    const uint32_t now = millis();
    if (lit_ && now - last_ >= config_.onMs) {
        write(false);
        last_ = now;
    } else if (!lit_ && now - last_ >= config_.periodMs) {
        write(true);
        last_ = now;
    }
}

namespace {
constexpr uint64_t kUsPerMs = 1000;
}

namespace power {

void radioOff() {
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
}

void cpuClock(uint32_t mhz) {
    setCpuFrequencyMhz(mhz);
}

void lightSleep(uint32_t maxMs, int wakePin, bool activeLow) {
    esp_sleep_enable_timer_wakeup(uint64_t(maxMs) * kUsPerMs);
    if (wakePin >= 0) {
        gpio_wakeup_enable(gpio_num_t(wakePin), activeLow ? GPIO_INTR_LOW_LEVEL : GPIO_INTR_HIGH_LEVEL);
        esp_sleep_enable_gpio_wakeup();
    } else {
        esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_GPIO);
    }
    esp_light_sleep_start();
}

void deepSleepOnButton(uint8_t wakePin, bool activeLow) {
#if CONFIG_IDF_TARGET_ESP32C3 || CONFIG_IDF_TARGET_ESP32C6 || CONFIG_IDF_TARGET_ESP32H2
    esp_deep_sleep_enable_gpio_wakeup(1ULL << wakePin,
                                      activeLow ? ESP_GPIO_WAKEUP_GPIO_LOW : ESP_GPIO_WAKEUP_GPIO_HIGH);
#else
    esp_sleep_enable_ext0_wakeup(gpio_num_t(wakePin), activeLow ? 0 : 1);
#endif
    esp_deep_sleep_start();
}

} // namespace power

} // namespace mrm
