#pragma once

#include <Arduino.h>
#include <SSD1306Wire.h>

#include "Clip.h"
#include "Icon.h"

namespace mrm {
namespace gfx {

// Ícones desenham na cor atual, sem fundo.
void drawIcon(SSD1306Wire& oled, int16_t x, int16_t y, const Icon& icon);

// O quadro do clip no instante dado. A origem de cada quadro é o ponto (x, y) mais o deslocamento dele.
inline void drawClip(SSD1306Wire& oled, int16_t x, int16_t y, const Clip& clip, uint32_t elapsedMs) {
    const Frame& frame = frameAt(clip, elapsedMs);
    drawIcon(oled, x + frame.dx, y + frame.dy, *frame.icon);
}

// Um retângulo preenchido com xadrez de 50%.
void ditherRect(SSD1306Wire& oled, int16_t x, int16_t y, int16_t w, int16_t h);
// Faixas pretas cobrindo o frame de cima pra baixo, o wipe de split flap.
void flapCover(SSD1306Wire& oled, int16_t w, uint8_t bands, int16_t bandHeight);

inline float clamp01(float x) { return x < 0 ? 0 : x > 1 ? 1 : x; }
inline float easeOut(float x) { return 1 - (1 - x) * (1 - x); }
inline float easeInOut(float x) {
    const float u = 2 - 2 * x;
    return x < 0.5f ? 2 * x * x : 1 - u * u / 2;
}
inline float stage(float t, float a, float b) { return clamp01((t - a) / (b - a)); }

} // namespace gfx
} // namespace mrm
