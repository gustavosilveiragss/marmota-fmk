#pragma once

#include "../Ssd1306Display.h"

namespace mrm {
namespace ui {

// Pecas de desenho para o OLED de 128x64.
constexpr int16_t kW = 128;
constexpr int16_t kH = 64;
constexpr int16_t kBar = 12;
constexpr int16_t kBodyY = 27;
constexpr int16_t kRow = 12;
constexpr uint32_t kMoveMs = 160; // cursor deslizando

float progressOf(uint32_t elapsed, uint32_t duration);
float lerp(float a, float b, float t);
float eased(uint32_t now, uint32_t at, uint32_t duration); // 1 se `at` for 0 (nunca aconteceu)
int16_t textWidth(Panel& o, const char* text);
void centered(Panel& o, const uint8_t* font, int16_t cx, int16_t y, const char* text);
void clipped(Panel& o, int16_t x, int16_t y, const char* text, int16_t maxWidth);
void clear(Panel& o, int16_t x, int16_t y, int16_t w, int16_t h);

struct Hint {
    const char* key;
    const char* label;
};
// Barra de instrucoes no topo: cada tecla num quadrado escuro e o que ela faz.
void hintBar(Panel& o, const Hint* items, uint8_t count);

} // namespace ui
} // namespace mrm
