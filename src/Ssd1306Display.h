#pragma once

#include <Arduino.h>
#include <SSD1306Wire.h>

namespace mrm {

class Ssd1306Display {
public:
    struct Config {
        uint8_t address = 0x3c;
        uint8_t sda = 5;
        uint8_t scl = 6;
        bool flip = true;
    };

    Ssd1306Display();
    explicit Ssd1306Display(const Config& config);

    void begin();
    void reinit();

    void clear();
    void show();
    void line(int16_t y, const String& text);
    void rightText(int16_t y, const String& text);
    void centered(int16_t y, const String& text);
    void batteryBadge(uint8_t percent);

    SSD1306Wire& raw() { return oled_; }

private:
    void applyDefaults();

    Config config_;
    SSD1306Wire oled_;
};

} // namespace mrm
