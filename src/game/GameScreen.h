#pragma once

#include "GameView.h"

namespace mrm {
namespace game {

// Desenha uma partida: tutorial (2 páginas), contagem, jogo, pausa e fim.
void screen(Panel& o, const GameView& v, uint32_t now, Lang lang);

} // namespace game
} // namespace mrm
