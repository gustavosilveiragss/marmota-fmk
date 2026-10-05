#include "Gfx.h"

namespace mrm {
namespace gfx {

void drawIcon(SSD1306Wire& oled, int16_t x, int16_t y, const Icon& icon) {
    const uint8_t bytesPerRow = (icon.w + 7) / 8;
    for (uint8_t row = 0; row < icon.h; ++row) {
        for (uint8_t col = 0; col < icon.w; ++col) {
            const uint8_t bits = pgm_read_byte(&icon.xbm[row * bytesPerRow + (col >> 3)]);
            if (!(bits & (1 << (col & 7))))
                continue;

            oled.setPixel(x + col, y + row);
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

} // namespace gfx
} // namespace mrm
