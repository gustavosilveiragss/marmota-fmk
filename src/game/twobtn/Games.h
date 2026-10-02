#pragma once

// O esquema de 2 botoes (A e B) e especifico do mp3. A base (Game, GameHost, GameStore, GameView e
// as telas) fica fora desta pasta e e generica: jogos com outra entrada ganham outra subpasta.

#include "Breakout.h"
#include "Impact.h"
#include "Snake.h"
#include "Text.h"

namespace mrm {
namespace game {
namespace twobtn {

constexpr uint8_t kCount = 3;

// Os jogos de 2 botoes, na ordem do menu, prontos para o GameHost.
struct Set {
    Snake snake;
    Impact impact;
    Breakout breakout;
    Game* const list[kCount] = {&snake, &impact, &breakout};

    Set() = default;
    Set(const Set&) = delete;
    Set& operator=(const Set&) = delete;
};

inline const Text& name(uint8_t index) {
    static constexpr Str kNames[kCount] = {Str::GameSnake, Str::GameImpact, Str::GameBreakout};
    return txt(kNames[index < kCount ? index : 0]);
}

} // namespace twobtn
} // namespace game
} // namespace mrm
