#pragma once

#include <Arduino.h>
#include <SSD1306Wire.h>

namespace mrm {

// SSD1306Wire com texto sem alocar: o drawString da lib so aceita String e faz copias no heap a
// cada chamada, o que fragmenta a memoria num redesenho de 20 quadros por segundo.
class Panel : public SSD1306Wire {
public:
    using SSD1306Wire::SSD1306Wire;

    // Uma linha de texto UTF-8, no alinhamento atual, sem quebra de linha.
    uint16_t drawText(int16_t x, int16_t y, const char* text) {
        const uint16_t length = strlen(text);
        return drawStringInternal(x, y, text, length, getStringWidth(text, length, true), true);
    }
};

class Ssd1306Display {
public:
public:
    struct Config {
        uint8_t address = 0x3c;
        uint8_t sda = 5;
        uint8_t scl = 6;
        bool flip = true;
        uint8_t contrast = 0; // 0 mantem o padrao da lib, mais baixo economiza bateria direto
    };

    Ssd1306Display();
    explicit Ssd1306Display(const Config& config);

    void begin();
    void reinit();

    // off() mantem o buffer e o controlador vivos (~5uA), entao on() traz o mesmo frame de volta.
    void on();
    void off();
    void setContrast(uint8_t contrast);

    void clear();
    void show();
    void line(int16_t y, const String& text);
    void rightText(int16_t y, const String& text);
    void centered(int16_t y, const String& text);
    void batteryBadge(uint8_t percent);

    Panel& raw() { return oled_; }

private:
    void applyDefaults();

    Config config_;
    Panel oled_;
};

} // namespace mrm
