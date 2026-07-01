#include "Power.h"

#include <WiFi.h>
#include <esp_sleep.h>

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

namespace power {

void radioOff() {
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
}

void lightSleep(uint32_t ms) {
    esp_sleep_enable_timer_wakeup(uint64_t(ms) * 1000ULL);
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
