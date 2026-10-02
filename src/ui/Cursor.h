#pragma once

#include <Arduino.h>

namespace mrm {
namespace ui {

// Linha selecionada de um menu e de onde veio, para a tela animar o deslize entre as duas.
struct Cursor {
    uint8_t pos = 0;
    uint8_t prev = 0;
    uint32_t at = 0; // millis do ultimo movimento, 0 = nunca

    void reset(uint8_t to = 0) {
        pos = prev = to;
        at = 0;
    }
    void move(uint8_t rows) {
        prev = pos;
        pos = (pos + 1) % rows;
        at = max<uint32_t>(millis(), 1);
    }
};

} // namespace ui
} // namespace mrm
