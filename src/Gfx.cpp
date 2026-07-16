#include "Gfx.h"

namespace mrm {
namespace gfx {

namespace {

enum class Blit { Solid, Reveal, Dither };

void blit(SSD1306Wire& oled, int16_t x, int16_t y, const Icon& icon, uint8_t scale, Blit mode, int16_t maxCol) {
    const uint8_t bytesPerRow = (icon.w + 7) / 8;
    for (uint8_t row = 0; row < icon.h; ++row) {
        for (uint8_t col = 0; col < icon.w; ++col) {
            if (mode == Blit::Reveal && col > maxCol)
                continue;
            if (mode == Blit::Dither && ((col + row) & 1))
                continue;
            const uint8_t bits = pgm_read_byte(&icon.xbm[row * bytesPerRow + (col >> 3)]);
            if (!(bits & (1 << (col & 7))))
                continue;
            const int16_t px = x + col * scale;
            const int16_t py = y + row * scale;
            for (uint8_t sy = 0; sy < scale; ++sy)
                for (uint8_t sx = 0; sx < scale; ++sx)
                    oled.setPixel(px + sx, py + sy);
        }
    }
}

} // namespace

void drawIcon(SSD1306Wire& oled, int16_t x, int16_t y, const Icon& icon, uint8_t scale) {
    blit(oled, x, y, icon, scale, Blit::Solid, 0);
}

void drawIconReveal(SSD1306Wire& oled, int16_t x, int16_t y, const Icon& icon, int16_t maxCol, uint8_t scale) {
    blit(oled, x, y, icon, scale, Blit::Reveal, maxCol);
}

void drawIconDither(SSD1306Wire& oled, int16_t x, int16_t y, const Icon& icon, uint8_t scale) {
    blit(oled, x, y, icon, scale, Blit::Dither, 0);
}

void arcUp(SSD1306Wire& oled, int16_t cx, int16_t cy, int16_t r) {
    if (r <= 0)
        return;
    const int16_t cut = cy - r / 4;
    int16_t x = r, y = 0, err = 1 - r;
    while (x >= y) {
        const int pts[8][2] = {{x, y}, {y, x}, {-x, y}, {-y, x}, {x, -y}, {y, -x}, {-x, -y}, {-y, -x}};
        for (const auto& p : pts) {
            const int py = cy + p[1];
            if (py <= cut)
                oled.setPixel(cx + p[0], py);
        }
        ++y;
        if (err < 0) {
            err += 2 * y + 1;
        } else {
            --x;
            err += 2 * (y - x) + 1;
        }
    }
}

void wifiArcs(SSD1306Wire& oled, int16_t cx, int16_t by, uint8_t frame, int16_t r) {
    oled.setPixel(cx, by); // a 5px plus base, fillCircle(r=1) would drop the bridging pixels
    oled.setPixel(cx - 1, by);
    oled.setPixel(cx + 1, by);
    oled.setPixel(cx, by - 1);
    oled.setPixel(cx, by + 1);
    if (frame >= 1)
        arcUp(oled, cx, by, r);
    if (frame >= 2)
        arcUp(oled, cx, by, lroundf(r * 1.9f));
    if (frame >= 3)
        arcUp(oled, cx, by, lroundf(r * 2.8f));
}

void ping(SSD1306Wire& oled, int16_t cx, int16_t cy, const int16_t* radii, uint8_t count) {
    for (uint8_t i = 0; i < count; ++i)
        if (radii[i] > 0)
            oled.drawCircle(cx, cy, radii[i]);
}

void thickLine(SSD1306Wire& oled, int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t width) {
    const int16_t dx = abs(x1 - x0), dy = -abs(y1 - y0);
    const int16_t sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1;
    const int16_t off = (width - 1) / 2;
    int16_t err = dx + dy;
    for (;;) {
        oled.fillRect(x0 - off, y0 - off, width, width);
        if (x0 == x1 && y0 == y1)
            break;
        const int16_t e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

void ditherRect(SSD1306Wire& oled, int16_t x, int16_t y, int16_t w, int16_t h) {
    for (int16_t j = 0; j < h; ++j)
        for (int16_t i = 0; i < w; ++i)
            if ((((x + i) + (y + j)) & 1) == 0)
                oled.setPixel(x + i, y + j);
}

void flapCover(SSD1306Wire& oled, int16_t w, uint8_t bands, int16_t bandHeight) {
    oled.setColor(BLACK);
    for (uint8_t b = 0; b < bands; ++b)
        oled.fillRect(0, b * bandHeight, w, bandHeight);
    oled.setColor(WHITE);
}

void invertFlash(SSD1306Wire& oled) {
    const size_t bytes = size_t(oled.getWidth()) * oled.getHeight() / 8;
    uint8_t* buf = oled.buffer;
    for (size_t i = 0; i < bytes; ++i)
        buf[i] = ~buf[i];
}

} // namespace gfx
} // namespace mrm
