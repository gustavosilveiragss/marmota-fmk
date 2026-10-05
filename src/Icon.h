#pragma once

#include <stdint.h>

namespace mrm {
namespace gfx {

struct Icon {
    uint8_t w;
    uint8_t h;
    const uint8_t* xbm; // LSB = pixel mais a esquerda, linhas com padding de byte
};

} // namespace gfx
} // namespace mrm
