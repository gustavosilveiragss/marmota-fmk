#pragma once

// O esquema de 2 botões (A e B) e específico do mp3. A base (Game, GameHost, GameStore, GameView e
// as telas) fica fora desta pasta e e genérica: jogos com outra entrada ganham outra subpasta.

#include "Breakout.h"
#include "Impact.h"
#include "Orbit.h"
#include "Rhythm.h"
#include "Snake.h"
#include "Stacker.h"
#include "Text.h"

namespace mrm {
namespace game {
namespace twobtn {

constexpr uint8_t kCount = 6;

// Os jogos de 2 botões, na ordem do menu, prontos para o GameHost.
struct Set {
    Snake snake;
    Impact impact;
    Breakout breakout;
    Stacker stacker;
    Rhythm rhythm;
    Orbit orbit;
    Game* const list[kCount] = {&snake, &impact, &breakout, &stacker, &rhythm, &orbit};

    Set() = default;
    Set(const Set&) = delete;
    Set& operator=(const Set&) = delete;
};

inline const Text& name(uint8_t index) {
    static constexpr const Text* kNames[kCount] = {&Str::GameSnake,   &Str::GameImpact, &Str::GameBreakout,
                                                   &Str::GameStacker, &Str::GameRhythm, &Str::GameOrbit};
    return *kNames[index < kCount ? index : 0];
}

} // namespace twobtn
} // namespace game
} // namespace mrm
