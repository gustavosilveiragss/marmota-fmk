#pragma once

#include "../ui/Cursor.h"
#include "Game.h"

namespace mrm {
namespace game {

// Estado de uma partida para desenhar um quadro. O GameHost preenche (fill), a tela só lê.
struct GameView {
    const Game* game = nullptr; // jogo em andamento
    GamePhase phase = GamePhase::Tutorial;
    uint8_t index = 0;
    uint16_t score = 0;
    uint16_t best = 0; // recorde do jogo em andamento
    bool record = false;
    bool fromPause = false;
    uint8_t page = 0; // página do tutorial
    uint32_t phaseAt = 0;
    ui::Cursor cursor;
};

} // namespace game
} // namespace mrm
