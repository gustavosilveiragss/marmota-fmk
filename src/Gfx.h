#pragma once

#include <Arduino.h>
#include <SSD1306Wire.h>

namespace mrm {
namespace gfx {

struct Icon {
    uint8_t w;
    uint8_t h;
    const uint8_t* xbm; // PROGMEM, LSB = leftmost pixel, rows byte-padded
};

// Icons draw in the current color with no background, scaled by an integer factor.
void drawIcon(SSD1306Wire& oled, int16_t x, int16_t y, const Icon& icon, uint8_t scale = 1);
// Only columns up to maxCol are drawn, a left to right reveal wipe.
void drawIconReveal(SSD1306Wire& oled, int16_t x, int16_t y, const Icon& icon, int16_t maxCol, uint8_t scale = 1);
// Every other pixel dropped in a checkerboard, for a dimmed, far away look.
void drawIconDither(SSD1306Wire& oled, int16_t x, int16_t y, const Icon& icon, uint8_t scale = 1);

// The top part of a ring, the arc that reads as a broadcast wave.
void arcUp(SSD1306Wire& oled, int16_t cx, int16_t cy, int16_t r);
// A base dot with up to three stacked arcs, gated by frame (0..3).
void wifiArcs(SSD1306Wire& oled, int16_t cx, int16_t by, uint8_t frame, int16_t r);
// Concentric rings, one per positive radius.
void ping(SSD1306Wire& oled, int16_t cx, int16_t cy, const int16_t* radii, uint8_t count);

void thickLine(SSD1306Wire& oled, int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t width);
// A rectangle filled with a 50% checkerboard.
void ditherRect(SSD1306Wire& oled, int16_t x, int16_t y, int16_t w, int16_t h);
// Black bands covering the frame top down, the split flap wipe.
void flapCover(SSD1306Wire& oled, int16_t w, uint8_t bands, int16_t bandHeight);
// Inverts every pixel in the buffer, a single frame flash under a redraw loop.
void invertFlash(SSD1306Wire& oled);

inline float clamp01(float x) { return x < 0 ? 0 : x > 1 ? 1 : x; }
inline float easeOut(float x) { return 1 - (1 - x) * (1 - x); }
inline float easeInOut(float x) {
    const float u = 2 - 2 * x;
    return x < 0.5f ? 2 * x * x : 1 - u * u / 2;
}
inline float stage(float t, float a, float b) { return clamp01((t - a) / (b - a)); }
// A gentle sine offset of amp pixels, one full cycle per periodMs.
inline int16_t breathe(uint32_t t, uint16_t periodMs, int16_t amp) {
    return int16_t(lroundf(sinf(float(t) / periodMs * TWO_PI) * amp));
}

} // namespace gfx
} // namespace mrm
