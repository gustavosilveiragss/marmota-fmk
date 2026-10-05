#pragma once

#include "../Locale.h"

namespace mrm {
namespace game {

// Textos das telas genéricas de jogo. As dicas desce/ok vêm de Locale.h.
namespace Str {
inline constexpr Text HintBack{"voltar", "back"};
inline constexpr Text HintNext{"avançar", "next"};
inline constexpr Text HintExit{"sair", "exit"};
inline constexpr Text HintPlay{"jogar", "play"};

inline constexpr Text Exit{"Sair", "Exit"};
inline constexpr Text Resume{"Continuar", "Resume"};
inline constexpr Text HowTo{"Como jogar", "How to play"};
inline constexpr Text PlayAgain{"Jogar de novo", "Play again"};
inline constexpr Text Paused{"Pausa", "Paused"};
inline constexpr Text Best{"recorde", "best"};
inline constexpr Text NewRecord{"novo recorde!", "new record!"};
} // namespace Str

inline const char* tr(Lang lang, const Text& text) {
    return lang == Lang::En ? text.en : text.ptBr;
}

} // namespace game
} // namespace mrm
