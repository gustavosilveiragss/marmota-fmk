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
    GameStacker,
    GameRhythm,
    GameOrbit,
    GoalSnake,
    GoalImpact,
    GoalBreakout,
    GoalStacker,
    GoalRhythm,
    GoalOrbit,
    KeyLeft,
    KeyRight,
    KeyUp,
    KeyDown,
    KeyDrop,
    KeyBrake,
    SnakeR1,
    SnakeR2,
    SnakeR3,
    ImpactR1,
    ImpactR2,
    ImpactR3,
    BreakoutR1,
    BreakoutR2,
    BreakoutR3,
    StackerR1,
    StackerR2,
    StackerR3,
    RhythmR1,
    RhythmR2,
    RhythmR3,
    OrbitR1,
    OrbitR2,
    OrbitR3,
    Count
};

inline constexpr Text kTexts[] = {
    {"Snake", "Snake"},
    {"Impacto", "Impact"},
    {"Breakout", "Breakout"},
    {"Torre", "Stacker"},
    {"Ritmo", "Rhythm"},
    {"Órbita", "Orbit"},
    {"coma sem se morder", "eat, don't bite yourself"},
    {"destrua as naves", "destroy the ships"},
    {"quebre todos os blocos", "break all the bricks"},
    {"empilhe os blocos", "stack the blocks"},
    {"toque no compasso", "tap to the beat"},
    {"passe pelas brechas", "pass through the gaps"},
    {"esquerda", "left"},
    {"direita", "right"},
    {"sobe", "up"},
    {"desce", "down"},
    {"solta", "drop"},
    {"freio", "brake"},
    {"Coma os quadrados", "Eat the squares"},
    {"As paredes dão a volta", "Walls wrap around"},
    {"Não bata em si mesma", "Don't hit yourself"},
    {"O tiro é automático", "Fire is automatic"},
    {"Desvie e destrua naves", "Dodge and shoot ships"},
    {"Item +: tiro duplo", "Item +: double shot"},
    {"Quebre todos os blocos", "Break all the bricks"},
    {"Use a ponta da raquete", "Use the paddle edge"},
    {"para mudar o ângulo", "to change the angle"},
    {"Solte o bloco", "Drop the block"},
    {"Sobra cai fora", "Overhang falls off"},
    {"Exato: bloco cresce", "Perfect: block grows"},
    {"Toque na linha", "Tap on the line"},
    {"Segure notas longas", "Hold long notes"},
    {"3 erros: fim", "3 misses: game over"},
    {"Ache a brecha", "Find the gap"},
    {"Segure para girar", "Hold to spin"},
    {"Encostou: fim", "Touch a wall: over"},
};
static_assert(sizeof(kTexts) / sizeof(kTexts[0]) == static_cast<size_t>(Str::Count), "kTexts precisa ter uma linha por Str");

constexpr const Text& txt(Str key) {
    return kTexts[static_cast<uint16_t>(key)];
}

} // namespace twobtn
} // namespace game
} // namespace mrm
