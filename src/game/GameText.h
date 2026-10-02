#pragma once

#include "../Locale.h"

namespace mrm {
namespace game {

// Textos das telas genericas de jogo (dicas, pausa, fim). A ordem da tabela segue o enum.
enum class Str : uint16_t {
    HintDown,
    HintOk,
    HintBack,
    HintNext,
    HintExit,
    HintPlay,
    Exit,
    Resume,
    HowTo,
    PlayAgain,
    Paused,
    Best,
    NewRecord,
    Count
};

inline constexpr Text kTexts[] = {
    {"desce", "down"},
    {"ok", "ok"},
    {"volta", "back"},
    {">", ">"},
    {"sair", "exit"},
    {"jogar", "play"},
    {"Sair", "Exit"},
    {"Continuar", "Resume"},
    {"Como jogar", "How to play"},
    {"Jogar de novo", "Play again"},
    {"Pausa", "Paused"},
    {"recorde", "best"},
    {"novo recorde!", "new record!"},
};
static_assert(sizeof(kTexts) / sizeof(kTexts[0]) == static_cast<size_t>(Str::Count), "kTexts precisa ter uma linha por Str");

inline const char* tr(Lang lang, const Text& text) {
    return lang == Lang::En ? text.en : text.ptBr;
}
inline const char* tr(Lang lang, Str key) {
    return tr(lang, kTexts[static_cast<uint16_t>(key)]);
}

} // namespace game
} // namespace mrm
