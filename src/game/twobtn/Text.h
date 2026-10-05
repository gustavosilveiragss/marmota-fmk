#pragma once

#include "../GameText.h"

namespace mrm {
namespace game {
namespace twobtn {

// Textos dos jogos de 2 botões: nome, objetivo, o que cada tecla faz e as regras.
namespace Str {
inline constexpr Text GameSnake{"Snake", "Snake"};
inline constexpr Text GameImpact{"Impacto", "Impact"};
inline constexpr Text GameBreakout{"Breakout", "Breakout"};
inline constexpr Text GameStacker{"Torre", "Stacker"};
inline constexpr Text GameRhythm{"Ritmo", "Rhythm"};
inline constexpr Text GameOrbit{"Órbita", "Orbit"};

inline constexpr Text GoalSnake{"coma sem se morder", "eat, don't bite yourself"};
inline constexpr Text GoalImpact{"destrua as naves", "destroy the ships"};
inline constexpr Text GoalBreakout{"quebre todos os blocos", "break all the bricks"};
inline constexpr Text GoalStacker{"empilhe os blocos", "stack the blocks"};
inline constexpr Text GoalRhythm{"toque no compasso", "tap to the beat"};
inline constexpr Text GoalOrbit{"passe pelas brechas", "pass through the gaps"};

inline constexpr Text KeyLeft{"esquerda", "left"};
inline constexpr Text KeyRight{"direita", "right"};
inline constexpr Text KeyUp{"sobe", "up"};
inline constexpr Text KeyDown{"desce", "down"};
inline constexpr Text KeyDrop{"solta", "drop"};
inline constexpr Text KeyBrake{"freio", "brake"};

inline constexpr Text SnakeR1{"Coma os quadrados", "Eat the squares"};
inline constexpr Text SnakeR2{"As paredes dão a volta", "Walls wrap around"};
inline constexpr Text SnakeR3{"Não bata em si mesma", "Don't hit yourself"};

inline constexpr Text ImpactR1{"O tiro é automático", "Fire is automatic"};
inline constexpr Text ImpactR2{"Desvie e destrua naves", "Dodge and shoot ships"};
inline constexpr Text ImpactR3{"Item +: tiro duplo", "Item +: double shot"};

inline constexpr Text BreakoutR1{"Quebre todos os blocos", "Break all the bricks"};
inline constexpr Text BreakoutR2{"Use a ponta da raquete", "Use the paddle edge"};
inline constexpr Text BreakoutR3{"para mudar o ângulo", "to change the angle"};

inline constexpr Text StackerR1{"Solte o bloco", "Drop the block"};
inline constexpr Text StackerR2{"Sobra cai fora", "Overhang falls off"};
inline constexpr Text StackerR3{"Exato: bloco cresce", "Perfect: block grows"};

inline constexpr Text RhythmR1{"Toque na linha", "Tap on the line"};
inline constexpr Text RhythmR2{"Segure notas longas", "Hold long notes"};
inline constexpr Text RhythmR3{"3 erros: fim", "3 misses: game over"};

inline constexpr Text OrbitR1{"Ache a brecha", "Find the gap"};
inline constexpr Text OrbitR2{"Segure para girar", "Hold to spin"};
inline constexpr Text OrbitR3{"Encostou: fim", "Touch a wall: over"};
} // namespace Str

} // namespace twobtn
} // namespace game
} // namespace mrm
