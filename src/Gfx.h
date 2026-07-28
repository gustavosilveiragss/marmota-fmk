#pragma once

#include <Arduino.h>
#include <SSD1306Wire.h>

namespace mrm {
namespace gfx {

struct Icon {
    uint8_t w;
    uint8_t h;
    const uint8_t* xbm; // PROGMEM, LSB = pixel mais a esquerda, linhas com padding de byte
};

// Icones desenham na cor atual sem fundo, escalados por um fator inteiro.
void drawIcon(SSD1306Wire& oled, int16_t x, int16_t y, const Icon& icon, uint8_t scale = 1);
// So as colunas ate maxCol sao desenhadas, um reveal da esquerda pra direita.
void drawIconReveal(SSD1306Wire& oled, int16_t x, int16_t y, const Icon& icon, int16_t maxCol, uint8_t scale = 1);
// Um pixel sim outro nao em xadrez, pra um visual apagado e distante.
void drawIconDither(SSD1306Wire& oled, int16_t x, int16_t y, const Icon& icon, uint8_t scale = 1);

// A parte de cima de um anel, o arco que le como onda de broadcast.
void arcUp(SSD1306Wire& oled, int16_t cx, int16_t cy, int16_t r);
// Um ponto base com ate tres arcos empilhados, controlados por frame (0..3).
void wifiArcs(SSD1306Wire& oled, int16_t cx, int16_t by, uint8_t frame, int16_t r);
// Aneis concentricos, um por raio positivo.
void ping(SSD1306Wire& oled, int16_t cx, int16_t cy, const int16_t* radii, uint8_t count);

void thickLine(SSD1306Wire& oled, int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t width);
// Um retangulo preenchido com xadrez de 50%.
void ditherRect(SSD1306Wire& oled, int16_t x, int16_t y, int16_t w, int16_t h);
// Faixas pretas cobrindo o frame de cima pra baixo, o wipe de split flap.
void flapCover(SSD1306Wire& oled, int16_t w, uint8_t bands, int16_t bandHeight);
// Inverte todo pixel do buffer, um flash de um frame dentro de um loop de redraw.
void invertFlash(SSD1306Wire& oled);

inline float clamp01(float x) { return x < 0 ? 0 : x > 1 ? 1 : x; }
inline float easeOut(float x) { return 1 - (1 - x) * (1 - x); }
inline float easeInOut(float x) {
    const float u = 2 - 2 * x;
    return x < 0.5f ? 2 * x * x : 1 - u * u / 2;
}
inline float stage(float t, float a, float b) { return clamp01((t - a) / (b - a)); }
// Um deslocamento senoidal suave de amp pixels, um ciclo inteiro por periodMs.
inline int16_t breathe(uint32_t t, uint16_t periodMs, int16_t amp) {
    return int16_t(lroundf(sinf(float(t) / periodMs * TWO_PI) * amp));
}

} // namespace gfx
} // namespace mrm
