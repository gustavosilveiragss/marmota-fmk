#pragma once

#include "Cursor.h"
#include "Draw.h"

namespace mrm {
namespace ui {

// Uma linha de menu: rótulo a esquerda e, a direita, um interruptor, um texto ou barras de sinal.
struct Row {
    const char* label = "";
    const char* value = nullptr;
    int8_t toggle = -1; // -1 sem interruptor, 0 desligado, 1 ligado
    float toggleMove = 1;
    bool check = false;
    int8_t rssi = 0; // 0 sem sinal
};

struct Menu {
    const Hint* hints;
    uint8_t hintCount;
    const char* title;
    const Row* rows;
    uint8_t count;
};

// Barra de instruções, título e uma lista rolável de 3 linhas com o cursor deslizando.
void menuScreen(Panel& o, const Cursor& cursor, uint32_t now, const Menu& m);

} // namespace ui
} // namespace mrm
