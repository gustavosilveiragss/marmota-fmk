#pragma once

#include <Arduino.h>

namespace mrm {
namespace game {

// O que o GameHost precisa guardar entre partidas e reboots. Quem implementa decide onde (NVS,
// arquivo) e como espaça a escrita.
class GameStore {
public:
    virtual uint16_t best(uint8_t game) const = 0;
    // Só grava (e devolve true) quando a pontuação e maior que o recorde.
    virtual bool setBest(uint8_t game, uint16_t score) = 0;
    // O tutorial de cada jogo aparece só na primeira vez. Depois fica em Como jogar.
    virtual bool tutorialSeen(uint8_t game) const = 0;
    virtual void setTutorialSeen(uint8_t game) = 0;

protected:
    ~GameStore() = default;
};

} // namespace game
} // namespace mrm
