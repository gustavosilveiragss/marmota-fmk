#pragma once

#include "Icon.h"

namespace mrm {

// Deliberadamente sem definição: chamada em tempo de compilação vira erro de build.
void bitmapTextDoesNotMatchSize();

// Desenho em ASCII-art ('#' ligado, '.' desligado, espaços e quebras de linha ignorados) convertido em XBM
// na compilação. Declare como `inline constexpr` para os bytes ficarem em .rodata.
template <int Width, int Height>
struct Bitmap {
    static constexpr int kBytesPerRow = (Width + 7) / 8;
    uint8_t bytes[kBytesPerRow * Height] = {};

    constexpr gfx::Icon icon() const { return {uint8_t(Width), uint8_t(Height), bytes}; }
};

template <int Width, int Height>
constexpr Bitmap<Width, Height> bitmap(const char* art) {
    Bitmap<Width, Height> result;
    int pixel = 0;
    for (; *art; ++art) {
        if (*art == ' ' || *art == '\n' || *art == '\r' || *art == '\t')
            continue;

        if ((*art != '#' && *art != '.') || pixel >= Width * Height) {
            bitmapTextDoesNotMatchSize();
            return result;
        }

        if (*art == '#') {
            const int row = pixel / Width;
            const int col = pixel % Width;
            result.bytes[row * Bitmap<Width, Height>::kBytesPerRow + col / 8] |= uint8_t(1 << (col % 8));
        }

        ++pixel;
    }

    if (pixel != Width * Height)
        bitmapTextDoesNotMatchSize();
    return result;
}

} // namespace mrm
