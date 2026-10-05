#pragma once

#include <Arduino.h>

namespace mrm {

enum class Lang : uint8_t { PtBr,
                            En };
constexpr uint8_t kLangCount = 2;

// Um texto em todos os idiomas, na ordem de Lang. Pode ser formato de printf: a ordem dos
// argumentos têm de ser igual nos dois idiomas.
struct Text {
    const char* ptBr;
    const char* en;
};

// Dicas de teclas iguais em todo device e jogo (a de voltar difere: "volta" no device, "voltar" nos jogos).
inline constexpr Text HintDown{"desce", "down"};
inline constexpr Text HintOk{"ok", "ok"};

// Guarda o idioma escolhido e traduz cada Text. Quem persiste o idioma entre boots é o device, por
// exemplo no Config.
class Locale {
public:
    void set(Lang lang) { lang_ = lang; }
    Lang lang() const { return lang_; }

    const char* operator()(const Text& text) const { return lang_ == Lang::En ? text.en : text.ptBr; }

    static const char* code(Lang lang);                // "pt-BR", "en" (BCP 47)
    static const char* nativeName(Lang lang);          // "Português", "English"
    static bool fromCode(const char* code, Lang& out); // aceita "pt", "pt-br", "en-US", ...

private:
    Lang lang_ = Lang::PtBr;
};

} // namespace mrm
