#pragma once

#include <Arduino.h>

namespace mrm {

enum class Lang : uint8_t { PtBr,
                            En };
constexpr uint8_t kLangCount = 2;

// Um texto em todos os idiomas, na ordem de Lang. Pode ser formato de printf: a ordem dos
// argumentos tem de ser igual nos dois idiomas.
struct Text {
    const char* ptBr;
    const char* en;
};

// Traduz chaves de um catalogo do proprio device (uma tabela de Text indexada por um enum). Quem
// guarda o idioma escolhido entre boots e o device, por exemplo no Config.
class Locale {
public:
    Locale(const Text* table, uint16_t count)
        : table_(table)
        , count_(count) {}

    void set(Lang lang) { lang_ = lang; }
    Lang lang() const { return lang_; }

    // Texto no idioma atual; chave fora da tabela devolve "?".
    const char* operator[](uint16_t key) const;
    template <typename Key>
    const char* operator()(Key key) const { return (*this)[static_cast<uint16_t>(key)]; }

    static const char* code(Lang lang);                // "pt-BR", "en" (BCP 47)
    static const char* nativeName(Lang lang);          // "Português", "English"
    static bool fromCode(const char* code, Lang& out); // aceita "pt", "pt-br", "en-US", ...

private:
    const Text* table_;
    uint16_t count_;
    Lang lang_ = Lang::PtBr;
};

} // namespace mrm
