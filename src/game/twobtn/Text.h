#pragma once

#include "../GameText.h"

namespace mrm {
namespace game {
namespace twobtn {

// Textos dos jogos de 2 botoes: nome, objetivo, o que cada tecla faz e as regras.
enum class Str : uint16_t {
    GameSnake,
    GameImpact,
    GameBreakout,
    GoalSnake,
    GoalImpact,
    GoalBreakout,
    KeyLeft,
    KeyRight,
    KeyUp,
    KeyDown,
    SnakeR1,
    SnakeR2,
    SnakeR3,
    ImpactR1,
    ImpactR2,
    ImpactR3,
    BreakoutR1,
    BreakoutR2,
    BreakoutR3,
    Count
};

inline constexpr Text kTexts[] = {
    {"Snake", "Snake"},
    {"Impacto", "Impact"},
    {"Breakout", "Breakout"},
    {"coma sem se morder", "eat, don't bite yourself"},
    {"destrua as naves", "destroy the ships"},
    {"quebre todos os blocos", "break all the bricks"},
    {"esquerda", "left"},
    {"direita", "right"},
    {"sobe", "up"},
    {"desce", "down"},
    {"Coma os quadrados", "Eat the squares"},
    {"As paredes dão a volta", "Walls wrap around"},
    {"Não bata em si mesma", "Don't hit yourself"},
    {"O tiro é automático", "Fire is automatic"},
    {"Desvie e destrua naves", "Dodge and shoot ships"},
    {"Item +: tiro duplo", "Item +: double shot"},
    {"Quebre todos os blocos", "Break all the bricks"},
    {"Use a ponta da raquete", "Use the paddle edge"},
    {"para mudar o ângulo", "to change the angle"},
};
static_assert(sizeof(kTexts) / sizeof(kTexts[0]) == static_cast<size_t>(Str::Count), "kTexts precisa ter uma linha por Str");

constexpr const Text& txt(Str key) {
    return kTexts[static_cast<uint16_t>(key)];
}

} // namespace twobtn
} // namespace game
} // namespace mrm
