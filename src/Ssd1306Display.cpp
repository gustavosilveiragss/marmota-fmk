#include "Ssd1306Display.h"

namespace mrm {

Ssd1306Display::Ssd1306Display()
    : Ssd1306Display(Config{}) {}

Ssd1306Display::Ssd1306Display(const Config& config)
    : config_(config)
    , oled_(config.address, config.sda, config.scl) {}

void Ssd1306Display::begin() {
    oled_.init();
    applyDefaults();
}

void Ssd1306Display::reinit() {
    begin();
}

void Ssd1306Display::applyDefaults() {
    if (config_.flip)
        oled_.flipScreenVertically();
    oled_.setFont(ArialMT_Plain_10);
    oled_.setColor(WHITE);
    oled_.setTextAlignment(TEXT_ALIGN_LEFT);
}

void Ssd1306Display::clear() {
    oled_.clear();
}

void Ssd1306Display::show() {
    oled_.display();
}

void Ssd1306Display::line(int16_t y, const String& text) {
    oled_.setTextAlignment(TEXT_ALIGN_LEFT);
    oled_.drawString(0, y, text);
}

void Ssd1306Display::rightText(int16_t y, const String& text) {
    oled_.setTextAlignment(TEXT_ALIGN_RIGHT);
    oled_.drawString(oled_.getWidth(), y, text);
    oled_.setTextAlignment(TEXT_ALIGN_LEFT);
}

void Ssd1306Display::centered(int16_t y, const String& text) {
    oled_.setTextAlignment(TEXT_ALIGN_CENTER);
    oled_.drawString(oled_.getWidth() / 2, y, text);
    oled_.setTextAlignment(TEXT_ALIGN_LEFT);
}

void Ssd1306Display::batteryBadge(uint8_t percent) {
    rightText(0, String(percent) + "%");
}

} // namespace mrm
